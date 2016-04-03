/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND.
 */

#include <nan.h>

using namespace node;
using namespace v8;

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>

#include "tessocr.h"

NAN_METHOD(Tokenize) {
  ENTER_METHOD(3);

  Local<Object> buffer;
  unsigned char *data = NULL;
  size_t length = 0;
  std::string *path = NULL;

  if (info[0]->IsString()) {
    String::Utf8Value str(info[0]->ToString());
    path = new std::string(*str);
  } else if (Buffer::HasInstance(info[0])) {
    buffer = info[0]->ToObject();
    data = (unsigned char *) node::Buffer::Data(buffer);
    length = (size_t) node::Buffer::Length(buffer);
  } else {
    THROW_BAD_ARGS("Argument 0 must be a buffer or string");
  }

  if (!info[1]->IsObject()) {
    THROW_BAD_ARGS("Argument 1 must be a object");
  }
  Local<Object> options = info[1]->ToObject();
  CALLBACK_ARG(2);

  TokenizeBaton *baton = new TokenizeBaton();

  baton->options = ParseTokenizeOptions(options);

  if (path) {
    baton->path = path;
  } else {
    baton->buffer.Reset(buffer);
    baton->data = data;
    baton->length = length;
  }

  baton->callback = new Nan::Callback(callback);

  uv_work_t *req = new uv_work_t;
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, __eio_tokenize, __eio_tokenize_done);

  info.GetReturnValue().SetUndefined();
}

TokenizeOptions *ParseTokenizeOptions(const Local<Object> &options) {
  TokenizeOptions *answer = new TokenizeOptions();

  ParseOcrOptions(*answer, options);

  int level = tesseract::RIL_TEXTLINE;
  bool textOnly = true;

  Local<Value> level_value = Nan::Get(options, Nan::New("level").ToLocalChecked()).ToLocalChecked();
  if (level_value->IsNumber()) {
    level = (int) level_value->ToInteger()->Value();
  }

  Local<Value> text_only_value = Nan::Get(options, Nan::New("textOnly").ToLocalChecked()).ToLocalChecked();
  if (text_only_value->IsBoolean()) {
    textOnly = text_only_value->ToBoolean()->Value();
  }

  answer->level = level;
  answer->textOnly = textOnly;

  return answer;
}

void __eio_tokenize(uv_work_t *req) {
  TokenizeBaton *baton = static_cast<TokenizeBaton *>(req->data);
  TokenizeOptions *options = baton->options;

  tesseract::TessBaseAPI api;
  TIME_BEGIN();
  DEBUG_LOG("Initializing tesseract api with (language: '%s', tessdata: '%s')", options->language->data(),
            options->tessdata->data());
  int r = api.Init(options->tessdata->data(), options->language->data(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL,
                   false);

  if (r != 0) {
    snprintf(baton->errstring, sizeof(baton->errstring), "Tesseract init error with code %d", r);
    return;
  }
  TIME_COUNT();

  PIX *image = NULL;
  if (baton->data) {
    DEBUG_LOG("Reading image from buffer (%d bytes)", (int) baton->length);
    image = pixReadMem(baton->data, baton->length);
  } else if (baton->path) {
    DEBUG_LOG("Reading image from %s", baton->path->data());
    image = pixRead(baton->path->data());
  }

  if (!image) {
    snprintf(baton->errstring, sizeof(baton->errstring), "Tesseract read image error");
    return;
  }

  baton->dimensions[0] = image->w;
  baton->dimensions[1] = image->h;

  api.SetImage(image);

  if (options->psm) {
    DEBUG_LOG("Setting psm to %d", options->psm);
    api.SetPageSegMode((tesseract::PageSegMode) options->psm);
  }

  if (options->rects && options->rects->size() > 0) {
    for (std::list<Area *>::iterator it = options->rects->begin(); it != options->rects->end(); ++it) {
      Area rect;
      CalcRect(rect, **it, image->w, image->h);

      DEBUG_LOG("Processing image (%d, %d) with rect (%d, %d, %d, %d)", image->w, image->h, rect.x, rect.y, rect.w,
                rect.h);
      api.SetRectangle(rect.x, rect.y, rect.w, rect.h);

      TessTokenize(api, *baton->options, baton->results);
    }
  } else {
    TessTokenize(api, *baton->options, baton->results);
  }

  api.End();
  pixDestroy(&image);
  TIME_END();
}

void TessTokenize(tesseract::TessBaseAPI &api, TokenizeOptions &options, std::list<TokenizeResult *> &results) {
  DEBUG_LOG("Tokenize image components with (level: %d, text_only: %d)", options.level, options.textOnly);
  Boxa *boxes = api.GetComponentImages((tesseract::PageIteratorLevel) options.level, options.textOnly, NULL, NULL);
  DEBUG_LOG("Found %d image components.", boxes->n);
  for (int i = 0; i < boxes->n; i++) {
//    if (RuleFilter(options.ranges, boxes->n, i)) {
    BOX *box = boxaGetBox(boxes, i, L_CLONE);
    TokenizeResult *item = new TokenizeResult();
    item->x = box->x;
    item->y = box->y;
    item->w = box->w;
    item->h = box->h;
    item->confidence = api.MeanTextConf();
    results.push_back(item);
//    }
  }
}

void __eio_tokenize_done(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  TokenizeBaton *baton = static_cast<TokenizeBaton *>(req->data);

  if (status == UV_ECANCELED) {
    DEBUG_LOG("Tokenize of %p cancelled", req->data);
  } else {
    DEBUG_LOG("Tokenize done");
    if (!baton->callback->IsEmpty()) {

      Local<Value> argv[2];

      if (baton->errstring[0]) {
        argv[0] = Nan::Error(Nan::New<String>(baton->errstring).ToLocalChecked());
        argv[1] = Nan::Undefined();
      } else {
        Local<Object> result = Nan::New<Object>();
        // dimensions
        Local<Array> dimensions = Nan::New<Array>();
        Nan::Set(dimensions, 0, Nan::New<Integer>(baton->dimensions[0]));
        Nan::Set(dimensions, 1, Nan::New<Integer>(baton->dimensions[1]));
        Nan::Set(result, Nan::New<String>("dimensions").ToLocalChecked(), dimensions);
        // boxes
        Local<Array> boxes = Nan::New<Array>();
        unsigned int i = 0;
        for (std::list<TokenizeResult *>::iterator it = baton->results.begin(); it != baton->results.end(); ++it, i++) {
          Local<Object> item = Nan::New<Object>();
          Nan::Set(item, Nan::New<String>("x").ToLocalChecked(), Nan::New<Integer>((*it)->x));
          Nan::Set(item, Nan::New<String>("y").ToLocalChecked(), Nan::New<Integer>((*it)->y));
          Nan::Set(item, Nan::New<String>("w").ToLocalChecked(), Nan::New<Integer>((*it)->w));
          Nan::Set(item, Nan::New<String>("h").ToLocalChecked(), Nan::New<Integer>((*it)->h));
          Nan::Set(item, Nan::New<String>("confidence").ToLocalChecked(), Nan::New<Integer>((*it)->confidence));
          Nan::Set(boxes, i, item);
        }
        Nan::Set(result, Nan::New<String>("boxes").ToLocalChecked(), boxes);
        argv[0] = Nan::Undefined();
        argv[1] = result;
      }

      DEBUG_LOG("call tokenize callback");
      Nan::TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        Nan::FatalException(try_catch);
      }
    }
  }

  delete baton;
  delete req;
}


NAN_METHOD(Recognize) {
  ENTER_METHOD(3);

  Local<Object> buffer;
  unsigned char *data = NULL;
  size_t length = 0;
  std::string *path = NULL;

  if (info[0]->IsString()) {
    String::Utf8Value str(info[0]->ToString());
    path = new std::string(*str);
  } else if (Buffer::HasInstance(info[0])) {
    buffer = info[0]->ToObject();
    data = (unsigned char *) node::Buffer::Data(buffer);
    length = (size_t) node::Buffer::Length(buffer);
  } else {
    THROW_BAD_ARGS("Argument 0 must be a buffer or string");
  }

  if (!info[1]->IsObject()) {
    THROW_BAD_ARGS("Argument 1 must be a object");
  }
  Local<Object> options = info[1]->ToObject();
  CALLBACK_ARG(2);

  RecognizeBaton *baton = new RecognizeBaton();

  baton->options = ParseRecognizeOptions(options);

  if (path) {
    baton->path = path;
  } else {
    baton->buffer.Reset(buffer);
    baton->data = data;
    baton->length = length;
  }

  baton->callback = new Nan::Callback(callback);

  uv_work_t *req = new uv_work_t;
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, __eio_recognize, __eio_recognize_done);

  info.GetReturnValue().SetUndefined();
}

RecognizeOptions *ParseRecognizeOptions(const Local<Object> &options) {
  RecognizeOptions *answer = new RecognizeOptions();
  ParseOcrOptions(*answer, options);
  return answer;
}

void __eio_recognize(uv_work_t *req) {
  RecognizeBaton *baton = static_cast<RecognizeBaton *>(req->data);
  RecognizeOptions *options = baton->options;
  tesseract::TessBaseAPI api;
  TIME_BEGIN();
  DEBUG_LOG("Initing tesseract api with (language: '%s', tessdata: '%s')", options->language->data(),
            options->tessdata->data());
  int r = api.Init(options->tessdata->data(), options->language->data(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL,
                   false);

  if (r != 0) {
    snprintf(baton->errstring, sizeof(baton->errstring), "Tesseract init error with code %d", r);
    return;
  }

  TIME_COUNT();
  PIX *pix = NULL;
  if (baton->data) {
    DEBUG_LOG("Reading image from buffer (%d bytes)", (int) baton->length);
    pix = pixReadMem(baton->data, baton->length);
  } else if (baton->path) {
    DEBUG_LOG("Reading image from %s", baton->path->data());
    pix = pixRead(baton->path->data());
  }

  if (!pix) {
    snprintf(baton->errstring, sizeof(baton->errstring), "Tesseract read image error");
    return;
  }

  api.SetImage(pix);

  baton->result.clear();
  if (options->rects) {
    for (std::list<Area *>::iterator it = options->rects->begin(); it != options->rects->end(); ++it) {
      Area rect;
      CalcRect(rect, **it, pix->w, pix->h);

      DEBUG_LOG("Processing image (%d, %d) with rect (%d, %d, %d, %d)", pix->w, pix->h, rect.x, rect.y, rect.w, rect.h);
      api.SetRectangle(rect.x, rect.y, rect.w, rect.h);

      baton->result.append(api.GetUTF8Text());
    }
  } else {
    baton->result.append(api.GetUTF8Text());
  }

  TIME_END();
  DEBUG_LOG("Recognize complete");

  api.End();
  pixDestroy(&pix);
}

void __eio_recognize_done(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  RecognizeBaton *baton = static_cast<RecognizeBaton *>(req->data);

  if (status == UV_ECANCELED) {
    DEBUG_LOG("Recognize of %p cancelled", req->data);
  } else {
    DEBUG_LOG("Recognize done");

    if (!baton->callback->IsEmpty()) {

      Local<Value> argv[2];

      if (baton->errstring[0]) {
        argv[0] = Nan::Error(Nan::New<String>(baton->errstring).ToLocalChecked());
        argv[1] = Nan::Undefined();
      } else {
        Local<Value> result = Nan::Undefined();
        if (baton->result.length()) {
          result = Nan::New<String>(baton->result.c_str()).ToLocalChecked();
        }
        argv[0] = Nan::Undefined();
        argv[1] = result;
      }

      DEBUG_LOG("call tokenize callback");
      Nan::TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        Nan::FatalException(try_catch);
      }
    }
  }

  delete baton;
  delete req;
}


void ParseOcrOptions(OcrOptions &target, const Local<Object> &options) {
  std::string *language = new std::string("eng");
  std::string *tessdata = new std::string("/usr/local/share/tessdata/");
  int psm = tesseract::PSM_AUTO;
  std::list<Area *> *rects = NULL;

  Local<Value> lang_value = Nan::Get(options, Nan::New("language").ToLocalChecked()).ToLocalChecked();
  if (lang_value->IsString()) {
    String::Utf8Value str(lang_value);
    if (str.length() > 0) {
      language->clear();
      language->append(*str);
    }
  }

  Local<Value> tessdata_value = Nan::Get(options, Nan::New("tessdata").ToLocalChecked()).ToLocalChecked();
  if (tessdata_value->IsString()) {
    String::Utf8Value str(tessdata_value);
    if (str.length() < 4095) {
      tessdata->clear();
      tessdata->append(*str);
    }
  }

  Local<Value> psm_value = Nan::Get(options, Nan::New("psm").ToLocalChecked()).ToLocalChecked();
  if (psm_value->IsNumber()) {
    psm = (int) psm_value->ToInteger()->Value();
  }

  Local<Value> rects_value = Nan::Get(options, Nan::New("rects").ToLocalChecked()).ToLocalChecked();
  if (rects_value->IsArray()) {
    Handle<Object> rects_object = rects_value->ToObject();
    unsigned int len = rects_object->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    rects = new std::list<Area *>();
    DEBUG_LOG("Using rects: %d", len);
    for (unsigned int i = 0; i < len; i++) {
      Local<Value> r = rects_object->Get(i);
      if (r->IsArray()) {
        Local<Object> obj = r->ToObject();
        unsigned int n = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
        if (n == 4) {
          Area *rect = new Area();
          rect->x = (int) obj->Get(0)->ToInteger()->Value();
          rect->y = (int) obj->Get(1)->ToInteger()->Value();
          rect->w = (int) obj->Get(2)->ToInteger()->Value();
          rect->h = (int) obj->Get(3)->ToInteger()->Value();
          rects->push_back(rect);
          DEBUG_LOG("Area %d: (%d, %d, %d, %d)", i, rect->x, rect->y, rect->w, rect->h);
        }
      }
    }
  }

  target.tessdata = tessdata;
  target.language = language;
  target.psm = psm;
  target.rects = rects;

//  INFO_LOG("Process with options: (tessdata: %s, language: %s, psm: %d, rects: %d)",
//           tessdata->data(), language->data(), psm, rects ? (int) rects->size() : 0);
}

void CalcRect(Area &target, Area &source, int width, int height) {
  int x = source.x;
  int y = source.y;
  int w = source.w;
  int h = source.h;

  if (w < 0) {
    x += w;
    w = abs(w);
  }
  if (x < 0) x += width;

  if (h < 0) {
    y += h;
    h = abs(h);
  }
  if (y < 0) y += height;

  if (x < 0) {
    w += x;
    x = 0;
  }

  if (x < 0) {
    h += y;
    y = 0;
  }

  if (x >= width || y >= height) {
    x = y = w = h = 0;
  }

  if (x + w > width) {
    w = width - x;
  }

  if (y + h > height) {
    h = height - y;
  }

  target.x = x;
  target.y = y;
  target.w = w;
  target.h = h;
}

extern "C" {
static void Initialize(Local<Object> target) {
  Nan::HandleScope scope;
  Nan::SetMethod(target, "tokenize", Tokenize);
  Nan::SetMethod(target, "recognize", Recognize);

  Nan::Set(target, Nan::New<String>("tesseractVersion").ToLocalChecked(),
           Nan::New<String>(tesseract::TessBaseAPI::Version()).ToLocalChecked());
}

NODE_MODULE(tessocr, Initialize) ;
}

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


void __eio_tokenize(uv_work_t *req) {
  TokenizeBaton *baton = static_cast<TokenizeBaton *>(req->data);
  tesseract::TessBaseAPI api;
  TIME_BEGIN();
  DEBUG_LOG("Initializing tesseract api with (lang: '%s', tessdata: '%s')", baton->language->data(),
            baton->tessdata->data());
  int r = api.Init(baton->tessdata->data(), baton->language->data(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL,
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

  if (baton->psm) {
    DEBUG_LOG("Setting psm to %d", baton->psm);
    api.SetPageSegMode((tesseract::PageSegMode) baton->psm);
  }

  if (baton->rect) {
    DEBUG_LOG("Setting rect to (%d, %d, %d, %d)", baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
    api.SetRectangle(baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
  }

  TIME_COUNT();
  DEBUG_LOG("Tokenize image components wirh (level: %d, text_only: %d)", baton->level, baton->textOnly);
  Boxa *boxes = api.GetComponentImages((tesseract::PageIteratorLevel) baton->level, baton->textOnly, NULL, NULL);
  DEBUG_LOG("Found %d image components.", boxes->n);
  TIME_COUNT();
  for (int i = 0; i < boxes->n; i++) {
    if (rule_filter(baton->rules, baton->rules_count, boxes->n, i)) {
      BOX *box = boxaGetBox(boxes, i, L_CLONE);
      TokenizeResult *item = new TokenizeResult();
      item->x = box->x;
      item->y = box->y;
      item->w = box->w;
      item->h = box->h;
      item->confidence = api.MeanTextConf();
      baton->results.push_back(item);
    }
  }
  api.End();
  pixDestroy(&pix);
  TIME_END();
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

      if(baton->errstring[0]) {
        argv[0] =  Nan::Error(Nan::New<String>(baton->errstring).ToLocalChecked());
        argv[1] = Nan::Undefined();
      } else {
        Local<Array> results = Nan::New<Array>();
        unsigned int i = 0;
        for(std::list<TokenizeResult*>::iterator it = baton->results.begin(); it != baton->results.end(); ++it, i++) {
          Local<Object> item = Nan::New<Object>();
          Nan::Set(item, Nan::New<String>("x").ToLocalChecked(), Nan::New<Integer>((*it)->x));
          Nan::Set(item, Nan::New<String>("y").ToLocalChecked(), Nan::New<Integer>((*it)->y));
          Nan::Set(item, Nan::New<String>("w").ToLocalChecked(), Nan::New<Integer>((*it)->w));
          Nan::Set(item, Nan::New<String>("h").ToLocalChecked(), Nan::New<Integer>((*it)->h));
          Nan::Set(item, Nan::New<String>("confidence").ToLocalChecked(), Nan::New<Integer>((*it)->confidence));
          Nan::Set(results, i, item);
        }
        argv[0] = Nan::Undefined();
        argv[1] = results;
      }

      DEBUG_LOG("call tokenize callback");
      Nan::TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        Nan::FatalException(try_catch);
      }
    }
  }

  baton->destroy();
  delete baton;
  delete req;
}

void __eio_recognize(uv_work_t *req) {
  RecognizeBaton *baton = static_cast<RecognizeBaton *>(req->data);
  tesseract::TessBaseAPI api;
  TIME_BEGIN();
  DEBUG_LOG("Initing tesseract api with (lang: '%s', tessdata: '%s')", baton->language->data(),
            baton->tessdata->data());
  int r = api.Init(baton->tessdata->data(), baton->language->data(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL,
                   false);
  TIME_COUNT();
  if (r == 0) {
    PIX *pix = NULL;
    if (baton->data) {
      DEBUG_LOG("Reading image from buffer (%d bytes)", (int) baton->length);
      pix = pixReadMem(baton->data, baton->length);
    } else if (baton->path) {
      DEBUG_LOG("Reading image from %s", baton->path->data());
      pix = pixRead(baton->path->data());
    }
    if (pix) {
      api.SetImage(pix);

      if (baton->psm) {
        DEBUG_LOG("Setting psm to %d", baton->psm);
        api.SetPageSegMode((tesseract::PageSegMode) baton->psm);
      }

      TIME_COUNT();
      if (baton->prm == PRM_TEXTLINE) {
        DEBUG_LOG("Recognizing in text line mode");
        Boxa *boxes = api.GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
        DEBUG_LOG("Found %d textline image components.", boxes->n);
        TIME_COUNT();
        DEBUG_LOG("Recognizing textline image components");
        std::string *result = new std::string();
        for (int i = 0; i < boxes->n; i++) {
          if (rule_filter(baton->rules, baton->rules_count, boxes->n, i)) {
            BOX *box = boxaGetBox(boxes, i, L_CLONE);
            api.SetRectangle(box->x, box->y, box->w, box->h);
            char *line = api.GetUTF8Text();
            result->append(line);
//        int conf = api.MeanTextConf();
//        fprintf(stdout, "Box[%d]: x=%d, y=%d, w=%d, h=%d, confidence: %d, text: %s", i, box->x, box->y, box->w, box->h, conf, line);
          }
        }
        baton->result = result;
      } else {
        DEBUG_LOG("Recognizing in page mode");
        if (baton->rect) {
          DEBUG_LOG("Setting rect to (%d, %d, %d, %d)", baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
          api.SetRectangle(baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
        }
        baton->result = new std::string(api.GetUTF8Text());
      }

      TIME_END();
      DEBUG_LOG("Recognize complete");

      api.End();
      pixDestroy(&pix);
    }
    else {
      DEBUG_LOG("Reading image error");
      baton->errcode = 2;
    }
  }
  else {
    DEBUG_LOG("Tesseract init error with %d", r);
    baton->errcode = r;
  }
}

void __eio_recognize_done(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  RecognizeBaton *baton = static_cast<RecognizeBaton *>(req->data);

  if (status == UV_ECANCELED) {
    DEBUG_LOG("Recognize of %p cancelled", req->data);
  } else {
    DEBUG_LOG("Recognize done");

    if (!baton->callback->IsEmpty()) {
      Local<Value> error = Nan::Undefined();
      if (baton->errcode != 0) {
        error = ocr_error(baton->errcode);
      }
      Local<Value> result = Nan::Undefined();
      if (baton->result) {
        result = Nan::New<String>(baton->result->data(), baton->result->length()).ToLocalChecked();
      }

      Local<Value> argv[] = {error, result};

      DEBUG_LOG("call recognize callback");
      Nan::TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        Nan::FatalException(try_catch);
      }
    }
  }

  baton->destroy();
  delete baton;
  delete req;
}

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

  // ********************************************************************************************************
  // * Decode Options
  // ********************************************************************************************************
  std::string *language = new std::string("eng");
  std::string *tessdata = new std::string("/usr/local/share/tessdata/");
  int psm = tesseract::PSM_AUTO;
  int level = tesseract::RIL_TEXTLINE;
  bool textOnly = true;
  int *rect = NULL;
  int **rules = NULL;
  unsigned int rules_count = 0;

  Local<Value> lang_value = Nan::Get(options, Nan::New("lang").ToLocalChecked()).ToLocalChecked();
  if (lang_value->IsString()) {
    String::Utf8Value str(lang_value);
    if (str.length() > 0) {
      language = new std::string(*str);
    }
  }

  Local<Value> tessdata_value = Nan::Get(options, Nan::New("tessdata").ToLocalChecked()).ToLocalChecked();
  if (tessdata_value->IsString()) {
    String::Utf8Value str(tessdata_value);
    if (str.length() < 4095) {
      tessdata = new std::string(*str);
    }
  }

  Local<Value> psm_value = Nan::Get(options, Nan::New("psm").ToLocalChecked()).ToLocalChecked();
  if (psm_value->IsNumber()) {
    psm = (int) psm_value->ToInteger()->Value();
  }

  Local<Value> level_value = Nan::Get(options, Nan::New("level").ToLocalChecked()).ToLocalChecked();
  if (level_value->IsNumber()) {
    level = (int) level_value->ToInteger()->Value();
  }

  Local<Value> text_only_value = Nan::Get(options, Nan::New("textOnly").ToLocalChecked()).ToLocalChecked();
  if (text_only_value->IsBoolean()) {
    textOnly = text_only_value->ToBoolean()->Value();
  }

  Local<Value> box_value = Nan::Get(options, Nan::New("rect").ToLocalChecked()).ToLocalChecked();
  if (box_value->IsArray()) {
    Handle<Object> box = box_value->ToObject();
    unsigned int len = box->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    if (len == 4) {
      rect = new int[len];
      for (unsigned int i = 0; i < len; i++) {
        Local<Value> element = box->Get(i);
        if (element->IsNumber()) {
          rect[i] = (int) element->ToInteger()->Value();
        }
      }
    }
  }

  Local<Value> lines_value = Nan::Get(options, Nan::New("lines").ToLocalChecked()).ToLocalChecked();
  if (lines_value->IsArray()) {
    Handle<Object> lines_object = lines_value->ToObject();
    unsigned int len = rules_count = lines_object->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    rules = new int *[len];
    INFO_LOG("Init lines: %d", len);
    for (unsigned int i = 0; i < len; i++) {
      rules[i] = 0;
      Local<Value> rule = lines_object->Get(i);
      if (rule->IsNumber()) {
        rules[i] = new int[2];
        rules[i][0] = rule->ToObject()->Int32Value();
        rules[i][1] = rules[i][0];
      } else if (rule->IsArray()) {
        Local<Object> obj = rule->ToObject();
        unsigned int n = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
        if (n > 0) {
          rules[i] = new int[2];
          rules[i][0] = (int) obj->Get(0)->ToInteger()->Value();
          rules[i][1] = n > 1 ? (int) obj->Get(1)->ToInteger()->Value() : rules[i][0];
        }
      }
      INFO_LOG("Line %d: (%d, %d)", i, rules[i][0], rules[i][1]);
    }
  }


  // ********************************************************************************************************

  TokenizeBaton *baton = new TokenizeBaton();
  strcpy(baton->errstring, "");

  baton->rect = rect;

  baton->language = language;
  baton->tessdata = tessdata;
  baton->psm = psm;
  baton->level = level;
  baton->textOnly = textOnly;
  baton->rules = rules;
  baton->rules_count = rules_count;

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

  // ********************************************************************************************************
  // * Decode Options
  // ********************************************************************************************************
  std::string *language = new std::string("eng");
  std::string *tessdata = new std::string("/usr/local/share/tessdata/");
  int psm = 3;
  int prm = 0;
  int *rect = NULL;
  int **rules = NULL;
  unsigned int rules_count = 0;

  Local<Value> lang_value = Nan::Get(options, Nan::New("lang").ToLocalChecked()).ToLocalChecked();
  if (lang_value->IsString()) {
    String::Utf8Value str(lang_value);
    if (str.length() > 0) {
      language = new std::string(*str);
    }
  }

  Local<Value> tessdata_value = Nan::Get(options, Nan::New("tessdata").ToLocalChecked()).ToLocalChecked();
  if (tessdata_value->IsString()) {
    String::Utf8Value str(tessdata_value);
    if (str.length() < 4095) {
      tessdata = new std::string(*str);
    }
  }

  Local<Value> psm_value = Nan::Get(options, Nan::New("psm").ToLocalChecked()).ToLocalChecked();
  if (psm_value->IsNumber()) {
    psm = (int) psm_value->ToInteger()->Value();
  }

  Local<Value> prm_value = Nan::Get(options, Nan::New("prm").ToLocalChecked()).ToLocalChecked();
  if (prm_value->IsNumber()) {
    prm = (int) prm_value->ToInteger()->Value();
  }

  Local<Value> box_value = Nan::Get(options, Nan::New("rect").ToLocalChecked()).ToLocalChecked();
  if (box_value->IsArray()) {
    Handle<Object> box = box_value->ToObject();
    unsigned int len = box->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    if (len == 4) {
      rect = new int[len];
      for (unsigned int i = 0; i < len; i++) {
        Local<Value> element = box->Get(i);
        if (element->IsNumber()) {
          rect[i] = (int) element->ToInteger()->Value();
        }
      }
    }
  }

  Local<Value> lines_value = Nan::Get(options, Nan::New("lines").ToLocalChecked()).ToLocalChecked();
  if (lines_value->IsArray()) {
    Handle<Object> lines_object = lines_value->ToObject();
    unsigned int len = rules_count = lines_object->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    rules = new int *[len];
    INFO_LOG("Init lines: %d", len);
    for (unsigned int i = 0; i < len; i++) {
      rules[i] = 0;
      Local<Value> rule = lines_object->Get(i);
      if (rule->IsNumber()) {
        rules[i] = new int[2];
        rules[i][0] = rule->ToObject()->Int32Value();
        rules[i][1] = rules[i][0];
      } else if (rule->IsArray()) {
        Local<Object> obj = rule->ToObject();
        unsigned int n = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
        if (n > 0) {
          rules[i] = new int[2];
          rules[i][0] = (int) obj->Get(0)->ToInteger()->Value();
          rules[i][1] = n > 1 ? (int) obj->Get(1)->ToInteger()->Value() : rules[i][0];
        }
      }
      INFO_LOG("Line %d: (%d, %d)", i, rules[i][0], rules[i][1]);
    }
  }


  // ********************************************************************************************************

  RecognizeBaton *baton = new RecognizeBaton();
  memset(baton, 0, sizeof(RecognizeBaton));

  baton->errcode = 0;
  baton->result = NULL;
  baton->rect = rect;

  baton->language = language;
  baton->tessdata = tessdata;
  baton->psm = psm;
  baton->prm = prm;
  baton->rules = rules;
  baton->rules_count = rules_count;

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

Local<Value> ocr_error(int code) {
  Local<Value> e = Nan::Error(Nan::New<String>("Tessocr throws an error").ToLocalChecked());
  e->ToObject()->Set(Nan::New<String>("code").ToLocalChecked(), Nan::New<Integer>(code));
  return e;
}

bool rule_filter(int **rules, int rules_count, int lines, int line) {
  if (!rules || !rules_count) return true;

  if (!lines || line >= lines || line < 0) return false;

  for (int i = 0; i < rules_count; i++) {
    int a = rules[i][0];
    int b = rules[i][1];
    a = a < 0 ? a + lines : a;
    b = b < 0 ? b + lines : b;

    if ((line >= a && line <= b) || (line >= b && line <= a)) {
      return true;
    }
  }

  return false;
}

extern "C" {
static void Initialize(Local<Object> target) {
  Nan::HandleScope scope;
  Nan::SetMethod(target, "tokenize", Tokenize);
  Nan::SetMethod(target, "recognize", Recognize);
}

NODE_MODULE(tessocr, Initialize) ;
}

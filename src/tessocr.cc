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

Nan::Persistent<FunctionTemplate> Tessocr::constructor_template;

Local<Value> ocr_error(int code) {
  Local<Value> e = Nan::Error(Nan::New<String>("Tessocr throws an error").ToLocalChecked());
  e->ToObject()->Set(Nan::New<String>("code").ToLocalChecked(), Nan::New<Integer>(code));
  return e;
}

void __eio_ocr(uv_work_t *req) {
  TessocrBaton *baton = static_cast<TessocrBaton *>(req->data);
  tesseract::TessBaseAPI api;
  DEBUG_LOG("Init tesseract api with (lang: '%s', tessdata: '%s')", baton->language->data(), baton->tessdata->data());
  int r = api.Init(baton->tessdata->data(), baton->language->data(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL, false);
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
        api.SetPageSegMode((tesseract::PageSegMode) baton->psm);
      }
      if (baton->rect) {
        api.SetRectangle(baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
      }
      baton->textresult = api.GetUTF8Text();
      api.End();
      pixDestroy(&pix);
    }
    else {
      baton->errcode = 2;
    }
  }
  else {
    baton->errcode = r;
  }
}

void __eio_ocr_done(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  TessocrBaton *baton = static_cast<TessocrBaton *>(req->data);

  if (status == UV_ECANCELED) {
    DEBUG_LOG("OCR of %p cancelled", req->data);
  } else {
    DEBUG_LOG("OCR done");

    if (!baton->callback->IsEmpty()) {
      Local<Value> error = Nan::Undefined();
      if (baton->errcode != 0) {
        error = ocr_error(baton->errcode);
      }

      Local<Value> argv[] = {error, Nan::New<String>(baton->textresult, strlen(baton->textresult)).ToLocalChecked()};

      DEBUG_LOG("call ocr callback");
      Nan::TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        Nan::FatalException(try_catch);
      }
    }
  }

  baton->reset();
  delete baton;
  delete req;
}

Tessocr::Tessocr() {
  DEBUG_LOG("Created Tessocr %p", this);
}

Tessocr::~Tessocr() {
  DEBUG_LOG("Freed Tessocr %p", this);
}

NAN_METHOD(Tessocr::New) {
  ENTER_CONSTRUCTOR(0);

  Tessocr *t = new Tessocr();

  t->Wrap(info.This());
  t->This.Reset(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Tessocr::Ocr) {
  ENTER_METHOD(Tessocr, 3);

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
  Local<Object> config = info[1]->ToObject();
  std::string *language = NULL;
  std::string *tessdata = NULL;
  int psm = 0;
  int *rect = NULL;

  Local<Value> lang_value = Nan::Get(config, Nan::New("lang").ToLocalChecked()).ToLocalChecked();
  if (lang_value->IsString()) {
    String::Utf8Value str(lang_value);
    if (str.length() > 0) {
      language = new std::string(*str);
    }
  }

  Local<Value> tessdata_value = Nan::Get(config, Nan::New("tessdata").ToLocalChecked()).ToLocalChecked();
  if (tessdata_value->IsString()) {
    String::Utf8Value str(tessdata_value);
    if (str.length() < 4095) {
      tessdata = new std::string(*str);
    }
  }

  Local<Value> psm_value = Nan::Get(config, Nan::New("psm").ToLocalChecked()).ToLocalChecked();
  if (psm_value->IsNumber()) {
    psm = (int) psm_value->ToInteger()->Value();
  }

  Local<Value> box_value = Nan::Get(config, Nan::New("rect").ToLocalChecked()).ToLocalChecked();
  if (box_value->IsArray()) {
    Handle<Object> box = box_value->ToObject();
    int len = box->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    if (len == 4) {
      rect = new int[len];
      for (int i = 0; i < len; i++) {
        Local<Value> element = box->Get(i);
        if (element->IsNumber()) {
          rect[i] = (int) element->ToInteger()->Value();
        }
      }
    }
  }

  CALLBACK_ARG(2);

  TessocrBaton *baton = new TessocrBaton();
  memset(baton, 0, sizeof(TessocrBaton));
  baton->errcode = 0;
  baton->textresult = NULL;
  baton->rect = rect;

  if (language) {
    baton->language = language;
  } else {
    baton->language = new std::string("eng");
  }

  if (tessdata) {
    baton->tessdata = tessdata;
  } else {
    baton->tessdata = new std::string("/usr/local/share/tessdata/");
  }

  if (psm) {
    baton->psm = psm;
  } else {
    baton->psm = 3;
  }

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

  uv_queue_work(uv_default_loop(), req, __eio_ocr, __eio_ocr_done);

  info.GetReturnValue().SetUndefined();
}


void Tessocr::Init(Local<Object> target) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

  tpl->SetClassName(Nan::New("Tessocr").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "ocr", Ocr);

  constructor_template.Reset(tpl);
  target->Set(Nan::New("Tessocr").ToLocalChecked(), tpl->GetFunction());

  INFO_LOG("Tesseract v%s with Leptonica", tesseract::TessBaseAPI::Version());
}

extern "C" {
static void Initialize(Local<Object> target) {
  Tessocr::Init(target);
}

NODE_MODULE(tessocr, Initialize) ;
}

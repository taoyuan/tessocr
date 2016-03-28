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

void __eio_ocr(uv_work_t *req) {
  TessocrBaton *baton = static_cast<TessocrBaton *>(req->data);
  tesseract::TessBaseAPI api;
  int r = api.Init(baton->tessdata, baton->language, tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL, false);
  if (r == 0) {
    PIX *pix = pixReadMem(baton->data, baton->length);
    if (pix) {
      api.SetImage(pix);
      if (baton->rect) {
        api.SetRectangle(baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
      }
      baton->textresult = api.GetUTF8Text();
      api.End();
      pixDestroy(&pix);
    }
    else {
      baton->error = 2;
    }
  }
  else {
    baton->error = r;
  }
}

void __eio_ocr_done(uv_work_t *req, int status) {
  Nan::HandleScope scope;
  TessocrBaton *baton = static_cast<TessocrBaton *>(req->data);

  if (status == UV_ECANCELED) {
    DEBUG_LOG("OCR of %p cancelled", req->data);
  } else {
    DEBUG_LOG("OCR done");

    Local<Value> argv[2];

    argv[0] = Nan::New(baton->error + status);
    argv[1] = Nan::New<String>(baton->textresult, strlen(baton->textresult)).ToLocalChecked();

    Nan::TryCatch try_catch;

    baton->callback->Call(2, argv);

    if (try_catch.HasCaught()) {
      Nan::FatalException(try_catch);
    }
  }

  baton->reset();
}

Tessocr::Tessocr() {
  DEBUG_LOG("Created Tessocr %p", this);
}

Tessocr::~Tessocr() {
  DEBUG_LOG("Freed Tessocr %p", this);
  baton.destory();
}

NAN_METHOD(Tessocr::New) {
  ENTER_CONSTRUCTOR(0);

  Tessocr *t = new Tessocr();

  memset(&t->baton, 0, sizeof(TessocrBaton));

  t->Wrap(info.This());
  t->This.Reset(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Tessocr::Ocr) {
  ENTER_METHOD(Tessocr, 3);

    if (that->baton.req) {
      THROW_ERROR("Tessocr is already active");
    }

  if (!Buffer::HasInstance(info[0])) {
    THROW_BAD_ARGS("Argument 0 must be a buffer");
  }
  Local<Object> buffer = info[0]->ToObject();
  unsigned char *data = (unsigned char *) node::Buffer::Data(buffer);
  size_t length = (size_t) node::Buffer::Length(buffer);

  if (!info[1]->IsObject()) {
    THROW_BAD_ARGS("Argument 1 must be a object");
  }
  Local<Object> config = info[1]->ToObject();
  char *language = NULL;
  char *tessdata = NULL;
  int *rect = NULL;

  Local<Value> lang_value = Nan::Get(config, Nan::New("lang").ToLocalChecked()).ToLocalChecked();
  if (lang_value->IsString()) {
    String::Utf8Value str(lang_value);
    if (str.length() == 3) {
      language = *str;
    }
  }

  Local<Value> tessdata_value = Nan::Get(config, Nan::New("tessdata").ToLocalChecked()).ToLocalChecked();
  if (tessdata_value->IsString()) {
    String::Utf8Value str(tessdata_value);
    if (str.length() < 4095) {
      tessdata = *str;
    }
  }

  Local<Value> box_value = Nan::Get(config, Nan::New("rect").ToLocalChecked()).ToLocalChecked();
  if (box_value->IsArray()) {
    Handle<Object> box = box_value->ToObject();
    int len = box->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    if (len == 4) {
      rect = new int[len];
      for (uint32_t i = 0; i < len; i++) {
        Local<Value> element = box->Get(i);
        if (element->IsNumber()) {
          rect[i] = (int) element->ToInteger()->Value();
        }
      }
    }
  }

  CALLBACK_ARG(2);

  TessocrBaton &baton = that->baton;
  baton.tessocr = that;
  baton.error = 0;
  baton.textresult = NULL;
  baton.rect = rect;

  if (language) {
    baton.language = language;
  } else {
    baton.language = strdup("eng");
  }

  if (tessdata) {
    baton.tessdata = tessdata;
  } else {
    baton.tessdata = strdup("/usr/local/share/tessdata/");
  }

  baton.callback = new Nan::Callback(callback);
  baton.buffer.Reset(buffer);
  baton.data = data;
  baton.length = length;

  uv_work_t *req = new uv_work_t;
  req->data = &baton;
  baton.req = req;

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

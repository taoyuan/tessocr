#ifndef TESSOCR_H
#define TESSOCR_H

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>

#include "helpers.h"

#define INFO

using namespace node;
using namespace v8;

class Tessocr;

struct TessocrBaton {
  int errcode;
  char* language;
  char* tessdata;
  char* textresult;
  int* rect;
  Nan::Callback *callback;
  char path[1024];
  Nan::Persistent<Object> buffer;
  unsigned char *data;
  size_t length;

  void reset() {
    buffer.Reset();
    data = 0;
    length = 0;

    errcode = 0;

    delete[] language;
    language = 0;
    delete[] tessdata;
    tessdata = 0;
    delete[] textresult;
    textresult = 0;
    if(rect) {
      delete[] rect;
      rect = 0;
    }
    if (callback) delete callback;
    callback = 0;
  }
};


class Tessocr: public Nan::ObjectWrap {
public:
  static void Init(Local<Object> exports);
  static NAN_METHOD(New);
  static NAN_METHOD(Ocr);

private:
  Tessocr();
  ~Tessocr();

private:
  Nan::Persistent<v8::Object> This;
  static Nan::Persistent<FunctionTemplate> constructor_template;
};

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
  Local<Function> callback; \
  if (info.Length() > (CALLBACK_ARG_IDX)) { \
    if (!info[CALLBACK_ARG_IDX]->IsFunction()) { \
      return Nan::ThrowTypeError("Argument " #CALLBACK_ARG_IDX " must be a function"); \
    } \
    callback = Local<Function>::Cast(info[CALLBACK_ARG_IDX]); \
  } \

#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "ntesseract [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
#define DEBUG_LOG(...)
#endif

#ifdef INFO
#define INFO_LOG(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#else
#define INFO_LOG(...)
#endif

#endif // TESSOCR_H

#ifndef TESSOCR_H
#define TESSOCR_H

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>

#include <tesseract/baseapi.h>

#include "helpers.h"

#define DEBUG
#define INFO
#define PROFILE
#define COMP

using namespace node;
using namespace v8;

class Tessocr;

enum PageRecognizeMode {
  PRM_PAGE,     // Page mode
  PRM_TEXTLINE  // Paragraph within a block.
};

struct TessocrBaton {
  tesseract::TessBaseAPI *api;

  int errcode;
  std::string *language;
  std::string *tessdata;
  // tesseract::PageSegMode
  int psm;
  // Page recognize mode
  // 0: full page recognize
  // 1: text line recognize
  // default is 0
  int prm;

  int* rect;

  std::string *path;
  Nan::Callback *callback;
  Nan::Persistent<Object> buffer;
  unsigned char *data;
  size_t length;

  std::string *result;

  void reset() {
    buffer.Reset();
    data = 0;
    length = 0;

    errcode = 0;
    psm = 0;

    if (language) delete language;
    language = 0;
    if (tessdata) delete tessdata;
    tessdata = 0;

    if(rect) {
      delete[] rect;
      rect = 0;
    }
    if (path) delete path;
    path = 0;
    if (callback) delete callback;
    callback = 0;

    if (result) delete result;
    result = 0;
  }
};


class Tessocr: public Nan::ObjectWrap {
public:
  static void Init(Local<Object> exports);
  static NAN_METHOD(New);
  static NAN_METHOD(Recognize);

private:
  Tessocr();
  ~Tessocr();

private:
  Nan::Persistent<v8::Object> This;
  static Nan::Persistent<FunctionTemplate> constructor_template;
};

#define CHECK_TESS(r) \
	if (r != 0) { \
		return Nan::ThrowError("Error code" #r); \
	}

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
  Local<Function> callback; \
  if (info.Length() > (CALLBACK_ARG_IDX)) { \
    if (!info[CALLBACK_ARG_IDX]->IsFunction()) { \
      return Nan::ThrowTypeError("Argument " #CALLBACK_ARG_IDX " must be a function"); \
    } \
    callback = Local<Function>::Cast(info[CALLBACK_ARG_IDX]); \
  } \

#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "tessocr [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
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

#ifdef PROFILE

#define TIME_BEGIN() \
  clock_t __start, __stop, __end; \
  __start = __stop = clock(); \

#define TIME_COUNT() \
  __end = clock(); \
	fprintf(stderr, "Elapse: %f\n", (__end - __stop) / (double) CLOCKS_PER_SEC ); \
 	__stop = clock(); \

#define TIME_END() \
  __end = clock(); \
  if (__stop > __start) TIME_COUNT() \
	fprintf(stderr, "Elapse all: %f\n", (__end - __start) / (double) CLOCKS_PER_SEC ); \

#else
#define TIME_BEGIN()
#define TIME_COUNT()
#define TIME_END()
#endif

#endif // TESSOCR_H

#ifndef TESSOCR_H
#define TESSOCR_H

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <list>

#include <tesseract/baseapi.h>

#include "helpers.h"

#define DEBUG
#define INFO
#define PROFILE
#define COMP

using namespace node;
using namespace v8;

#define ERROR_STRING_SIZE 1024

enum PageRecognizeMode {
  PRM_PAGE,     // Page mode
  PRM_TEXTLINE  // Paragraph within a block.
};

NAN_METHOD(Tokenize);

void __eio_tokenize(uv_work_t *req);

void __eio_tokenize_done(uv_work_t *req, int status);

NAN_METHOD(Recognize);

void __eio_recognize(uv_work_t *req);

void __eio_recognize_done(uv_work_t *req, int status);

struct Area {
  int x;
  int y;
  int w;
  int h;
};


struct TokenizeResult: Area {
  int confidence;
};

struct OcrBaton {
  char errstring[ERROR_STRING_SIZE];
  std::string *language;
  std::string *tessdata;
  int psm;
  int *rect;
  std::list<Area *> *rects;
  int **rules;
  int rules_count;
  std::string *path;
  Nan::Callback *callback;
  Nan::Persistent<Object> buffer;
  unsigned char *data;
  size_t length;

  virtual ~OcrBaton() {
    buffer.Reset();
    data = 0;
    length = 0;

    psm = 0;

    if (language) delete language;
    language = 0;
    if (tessdata) delete tessdata;
    tessdata = 0;

    if (rect) {
      delete[] rect;
    }
    rect = 0;

    if (rules) {
      for (int i = 0; i < rules_count; i++) {
        delete[] rules[i];
      }
      delete[] rules;
    }
    rules = 0;
    rules_count = 0;

    if (path) delete path;
    path = 0;
    if (callback) delete callback;
    callback = 0;

    if (rects) {
      for (std::list<Area *>::iterator it = rects->begin(); it != rects->end(); ++it) {
        delete *it;
      }
      delete rects;
      rects = 0;
    }
  }
};

struct TokenizeBaton: OcrBaton {
  int level;
  bool textOnly;
  std::list<TokenizeResult *> results;

  virtual ~TokenizeBaton() {
    level = 0;
    textOnly = 0;
    for (std::list<TokenizeResult *>::iterator it = results.begin(); it != results.end(); ++it) {
      delete *it;
    }
  }
};

struct RecognizeBaton:OcrBaton {
  int prm;

  std::string *result;

  virtual ~RecognizeBaton() {
    prm = 0;
    if (result) delete result;
    result = 0;
  }
};


bool rule_filter(int **rules, int rules_count, int lines, int line);

Local<Value> ocr_error(int code);

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

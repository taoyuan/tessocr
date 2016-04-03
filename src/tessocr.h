#ifndef TESSOCR_H
#define TESSOCR_H

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <list>

#include <tesseract/baseapi.h>

#include "helpers.h"

//#define DEBUG
//#define PROFILE
#define INFO

using namespace node;
using namespace v8;

#define ERROR_STRING_SIZE 1024

NAN_METHOD(Version);

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

struct TokenizeResult : Area {
  int confidence;
};

struct Range {
  int from;
  int to;

  Range() {}
  Range(int from, int to): from(from), to(to) {}
  Range(int from): from(from), to(from) {}
};

struct OcrOptions {
  std::string *language;
  std::string *tessdata;
  int psm;
  std::list<Area *> *rects;

  virtual ~OcrOptions() {
    if (language) delete language;
    if (tessdata) delete tessdata;
    if (rects) {
      for (std::list<Area *>::iterator it = rects->begin(); it != rects->end(); ++it) {
        delete *it;
      }
      delete rects;
    }
  }
};

struct TokenizeOptions : OcrOptions {
  // enable fast tokenization and using threshold as min white height
  int level;
  bool textOnly;
};

struct RecognizeOptions : OcrOptions {
};

struct OcrBaton {
  char errstring[ERROR_STRING_SIZE];
  std::string *path;
  Nan::Callback *callback;
  Nan::Persistent<Object> buffer;
  unsigned char *data;
  size_t length;

  OcrBaton() {
    strcpy(errstring, "");
  }

  virtual ~OcrBaton() {
    buffer.Reset();
    if (path) delete path;
    if (callback) delete callback;
  }
};

struct TokenizeBaton : OcrBaton {
  TokenizeOptions *options;
  int dimensions[2];
  std::list<TokenizeResult *> results;

  virtual ~TokenizeBaton() {
    if (options) delete options;
    for (std::list<TokenizeResult *>::iterator it = results.begin(); it != results.end(); ++it) {
      delete *it;
    }
  }
};

struct RecognizeBaton : OcrBaton {
  RecognizeOptions *options;
  std::string result;

  virtual ~RecognizeBaton() {
    if (options) delete options;
  }
};

void ParseOcrOptions(OcrOptions &target, const Local<Object> &options);

TokenizeOptions *ParseTokenizeOptions(const Local<Object> &options);

RecognizeOptions *ParseRecognizeOptions(const Local<Object> &options);

void TessTokenize(tesseract::TessBaseAPI &api, TokenizeOptions &options, std::list<TokenizeResult *> &results);

void CalcRect(Area &target, Area &source, int width, int height);

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

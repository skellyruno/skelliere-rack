#include "../tools/RackCaptureArgs.h"
#include <cassert>
int main(){using rackcapture::parse;
 assert(!parse({}).full);
 auto old=parse({"shots"});assert(old.full&&old.legacy&&old.output=="shots");
 auto quick=parse({"--output","shots"});assert(!quick.full&&!quick.legacy&&quick.output=="shots");
 auto full=parse({"--output","shots","--full"});assert(full.full&&!full.legacy);
 assert(!parse({"--output"}).error.empty());
 assert(!parse({"--output","--full"}).error.empty());
 assert(!parse({"--output","a","--output","b"}).error.empty());
 assert(!parse({"--nonsense"}).error.empty());
}

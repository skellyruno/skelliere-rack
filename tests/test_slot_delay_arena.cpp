#include "../Source/Dsp.h"
#include <cstdlib>
#include <cstdio>
using namespace skl;
static void check(bool b,const char* message){if(!b){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
struct Reference {std::vector<float> b;int w=0;explicit Reference(int n):b(n){}void clear(){std::fill(b.begin(),b.end(),0);w=0;}void write(float x){b[w]=x;if(++w==int(b.size()))w=0;}float read(float d){int n=int(b.size());d=std::clamp(d,1.f,float(n-2));int i=int(d);float f=d-i;int p=w-1-i;if(p<0)p+=n;int q=p-1;if(q<0)q+=n;return b[p]*(1-f)+b[q]*f;}};
int main(){SlotDelayArena a;DelayLine l,r,other;{SlotDelayPreparation p(a);l.prepare(127);r.prepare(127);}{SlotDelayPreparation p(a);other.prepare(511);}check(a.storage->size()==511,"maximum not sum");Reference x(127),y(127),z(511);
for(int cycle=0;cycle<20;++cycle){l.clear();r.clear();x.clear();y.clear();for(int i=0;i<900;++i){float v=float(std::sin(i*.13+cycle));l.write(v);r.write(-v);x.write(v);y.write(-v);float d=1.f+float(i%123)+.37f;check(l.read(d)==x.read(d)&&r.read(d)==y.read(d),"pair parity/reset/wrap");}other.clear();z.clear();for(int i=0;i<1500;++i){float v=float(std::cos(i*.07+cycle));other.write(v);z.write(v);float d=1.f+float(i%507)+.22f;check(other.read(d)==z.read(d),"reused storage parity/reset/wrap");}}
DelayLine separate;separate.prepare(127);for(int i=0;i<100;++i){separate.write(1);other.write(-1);}check(separate.read(20)==1&&other.read(20)==-1,"standalone isolation");std::puts("Slot delay arena: exact reference parity, disjoint lines, repeated switches, wrap, isolated standalone");}

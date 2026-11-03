#include <gui/common/CellBalance.hpp>
#include <assert.h>
#include <stdio.h>
int main(){using namespace cellbalance;
assert(permille(3,5)==600&&permille(-5,5)==-1000&&permille(30,20)==1000&&permille(7,0)==0);
Animator<3> a;bool u[3]={true,true,false};int64_t d[3]={4,-4,0};
a.target(u,d);assert(a.scaleMv()==5&&a.permille(0)==800&&a.permille(1)==-800&&a.permille(2)==0&&!a.usable(2));
d[0]=40;d[1]=-40;a.target(u,d);a.step();assert(a.permille(0)>0&&a.permille(0)<1000&&a.scaleMv()>5&&a.scaleMv()<50);
for(int i=0;i<400;i++){a.step();}assert(a.scaleMv()==50&&a.permille(0)==800);
d[0]=4;d[1]=-4;a.target(u,d);for(int i=0;i<30;i++)a.step();assert(a.scaleMv()>40);
for(int i=0;i<2000;i++){a.step();}assert(a.scaleMv()==5&&a.permille(0)==800);
puts("PASS cell balance eased bars, fast-rising and slowly relaxing scale");}

#include <gui/common/CellBalance.hpp>
#include <assert.h>
#include <stdio.h>
int main(){using namespace cellbalance;assert(scaleMv(0)==5&&scaleMv(3)==5&&scaleMv(-7)==10&&scaleMv(20)==20&&scaleMv(21)==50&&scaleMv(180)==200&&scaleMv(4000)==1000);assert(permille(3,5)==600&&permille(-5,5)==-1000&&permille(30,20)==1000&&permille(-10,20)==-500);puts("PASS cell balance dynamic scale steps and clamped permille");}

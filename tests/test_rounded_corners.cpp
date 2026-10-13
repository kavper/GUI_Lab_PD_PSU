#include <gui/common/RoundedCorners.hpp>
#include <assert.h>
#include <stdio.h>
int main(){for(int y=0;y<12;y++)for(int x=0;x<12;x++){unsigned outer=ui::corners::coverage(12,x,y),inner=ui::corners::coverage(10,x-2,y-2);assert(inner<=outer&&outer<=64);assert(outer==ui::corners::coverage(12,y,x));}for(int y=0;y<10;y++)for(int x=0;x<10;x++)assert(ui::corners::coverage(10,x,y)==ui::corners::coverage(10,y,x));assert(ui::corners::coverage(10,0,0)==0);assert(ui::corners::coverage(10,9,9)==64);puts("PASS antialias corner masks: symmetric coverage, inset inside outer, no negative border weight");}

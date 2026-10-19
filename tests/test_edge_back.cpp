#include <gui/common/EdgeBackGesture.hpp>
#include <assert.h>
#include <stdio.h>
int main(){
 EdgeBackGesture g;g.begin(20,200,true);assert(!g.move(26,201));assert(g.move(100,203));assert(g.finish());
 g.begin(29,200,true);assert(!g.move(200,200)&&!g.finish());
 g.begin(20,63,true);assert(!g.move(200,63)&&!g.finish());
 g.begin(20,200,false);assert(!g.move(200,200)&&!g.finish());
 g.begin(20,200,true);assert(!g.move(22,240));assert(!g.move(200,240)&&!g.finish()); /* vertical list scroll wins */
 g.begin(20,200,true);assert(g.move(100,200));assert(g.move(30,200));assert(!g.finish()); /* reversing cancels without rebuilding the editor */
 g.begin(20,200,true);assert(!g.move(-100,200)&&!g.finish());
 g.begin(20,200,true);assert(g.move(160,200));g.move(160,320);assert(!g.finish());
 puts("PASS edge back: content-only, left edge, direction, vertical scroll, disabled and rollback");
}

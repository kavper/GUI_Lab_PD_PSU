#include <gui/common/UsbPdObjects.hpp>
#include <assert.h>
#include <stdio.h>
int main(){
 auto fixed=ui::PdObject::decode((400u<<10)|300u);assert(fixed.valid&&fixed.minMv==20000&&fixed.maxMv==20000&&fixed.ma==3000);
 auto pps=ui::PdObject::decode(0xc0000000u|(210u<<17)|(33u<<8)|60u);assert(pps.valid&&pps.minMv==3300&&pps.maxMv==21000&&pps.ma==3000);
 auto battery=ui::PdObject::decode(0x40000000u|(400u<<20)|(100u<<10)|240u);assert(battery.valid&&battery.minMv==5000&&battery.maxMv==20000&&battery.mw==60000);
 auto variable=ui::PdObject::decode(0x80000000u|(400u<<20)|(100u<<10)|300u);assert(variable.valid&&variable.ma==3000);
 assert(!ui::PdObject::decode(0).valid&&!ui::PdObject::decode(0xd0000000u).valid);
 G4PdCapabilities c={};c.valid=1;c.flags=7;c.count=7;c.role=1;c.attach_generation=0x1234;c.ms=0xfffffff0u;
 c.rdo=(5u<<28)|(300u<<10)|300u;c.active_pdo=(400u<<10)|300u;c.pdo[4]=c.active_pdo;
 assert(ui::selectedPdObject(c)==5);c.pdo[4]^=1;assert(ui::selectedPdObject(c)==0);c.pdo[4]^=1;assert(ui::pdCapabilitiesCurrent(c,0x20,true,1,0x34));
 assert(!ui::pdCapabilitiesCurrent(c,0x20,true,1,0x35));assert(!ui::pdCapabilitiesCurrent(c,0x20,true,2,0x34));assert(!ui::pdCapabilitiesCurrent(c,0x20,false,1,0x34));
 assert(!ui::pdCapabilitiesCurrent(c,0x1000,true,1,0x34));
 char text[240];ui::formatPdRow(text,sizeof(text),c.active_pdo,5,true);assert(text[0]=='>'&&strstr(text,"20.00 V")&&strstr(text,"3.00 A"));
 ui::formatPdRequest(text,sizeof(text),c);assert(strstr(text,"PDO #5")&&strstr(text,"Max 3.00 A"));
 c.rdo|=1u<<27;ui::formatPdRequest(text,sizeof(text),c);assert(strstr(text,"Min 3.00 A"));
 c.rdo=(6u<<28)|(550u<<9)|40u;c.active_pdo=pps.raw;ui::formatPdRequest(text,sizeof(text),c);assert(strstr(text,"11.00 V")&&strstr(text,"2.00 A"));
 c.flags=1;ui::formatPdRequest(text,sizeof(text),c);assert(strstr(text,"No active RDO"));assert(!ui::selectedPdObject(c));
 c.flags=7;c.rdo=8u<<28;assert(!ui::selectedPdObject(c));
 puts("PASS PDO/RDO units, fixed/variable/battery/PPS, unknown APDO, giveback, selection and cable-session freshness");
}

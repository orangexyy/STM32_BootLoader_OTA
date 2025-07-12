#include "sys.h"

//THUMB指锟筋不支锟街伙拷锟斤拷锟斤拷锟�
//锟斤拷锟斤拷锟斤拷锟铰凤拷锟斤拷实锟斤拷执锟叫伙拷锟街革拷锟絎FI  
void WFI_SET(void)
{
	__ASM volatile("wfi");		  
}
//锟截憋拷锟斤拷锟斤拷锟叫讹拷
void INTX_DISABLE(void)
{		  
	__ASM volatile("cpsid i");
}
//锟斤拷锟斤拷锟斤拷锟斤拷锟叫讹拷
void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");		  
}
//锟斤拷锟斤拷栈锟斤拷锟斤拷址
//addr:栈锟斤拷锟斤拷址
__asm void MSR_MSP(u32 addr) 
{
    MSR MSP, r0 			//set Main Stack value
    BX r14
}

#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PDX(va)     (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

paddr_t page_translate(vaddr_t addr,bool iswrite);

uint32_t paddr_read(paddr_t addr, int len) {
  int r=is_mmio(addr);
  if(r==-1)
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  return mmio_read(addr,len,r);
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int r=is_mmio(addr);
  if(r==-1)
    memcpy(guest_to_host(addr), &data, len);
  else
    mmio_write(addr,len,data,r);
  
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  //return paddr_read(addr, len);
  if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){
    printf("error:data pass two pages:addr=0x%x,len=%d!\n",addr,len);
    assert(0);
  }
  else {
    paddr_t paddr=page_translate(addr,false);
    return paddr_read(paddr,len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  //paddr_write(addr, len, data);
  if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){
    printf("error:data pass two pages:addr=0x%x,len=%d!\n",addr,len);
    assert(0);
  }
  else {
    paddr_t paddr=page_translate(addr,true);
    return paddr_write(paddr,len,data);
  }
}

paddr_t page_translate(vaddr_t addr,bool iswrite){

  CR0 cr0=(CR0)cpu.CR0;
  if(cr0.paging&&cr0.protect_enable){
    CR3 cr3=(CR3)cpu.CR3;

    PDE* pgdirs=(PDE*)PTE_ADDR(cr3.val);
    PDE pde=(PDE)paddr_read((uint32_t)(pgdirs+PDX(addr)),4);
    Assert(pde.present,"addr=0x%x\n",addr);

    PTE* ptab=(PTE*)PTE_ADDR(pde.val);
    PTE pte=(PTE)paddr_read((uint32_t)(ptab+PTX(addr)),4);
    Assert(pte.present,"addr=0x%x\n",addr);

    pde.accessed=1;
    pte.accessed=1;
    if(iswrite)pte.dirty=1;

    paddr_t paddr=PTE_ADDR(pte.val)|OFF(addr);

    printf("vaddr=0x%x,paddr=0x%x\n",addr,paddr);

    return paddr;

  }
  return addr;

}

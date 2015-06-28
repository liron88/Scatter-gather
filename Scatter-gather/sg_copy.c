#include <stdlib.h>
#include "sg_copy.h"

/*
* sg_map        Map a memory buffer using a scatter-gather list
*
* @in buf       Pointer to buffer
* @in length    Buffer length in bytes
*
* @ret          A list of sg_entry elements mapping the input buffer
*
* @note         Make a scatter-gather list mapping a buffer into
*               a list of chunks mapping up to PAGE_SIZE bytes each.
*               All entries except the first one must be aligned on a
*               PAGE_SIZE address;
*/
sg_entry_t* sg_map(void* buf, int length)
{
  physaddr_t paddr = ptr_to_phys(buf);
  sg_entry_t* list_head;
  sg_entry_t* list_curr;
  sg_entry_t* list_prev;
  int remaining_length;
  int count = 0;
  
  if (length <= 0) return NULL;

  if (length >= 32)
  {
    while ((paddr + count) % PAGE_SIZE)
    {
      count++;
    }
  }
  else
  {
    count = length;
  }

  list_head = (sg_entry_t*)malloc(sizeof(sg_entry_t));
  list_head->paddr = paddr;
  list_head->count = count;

  list_prev = list_head;

  remaining_length = length - count;

  while (remaining_length > 0)
  {
    // calculate count and address
    count = remaining_length >= PAGE_SIZE ? PAGE_SIZE : remaining_length;
    paddr += count;

    // create new entry
    list_curr = (sg_entry_t*)malloc(sizeof(sg_entry_t));
    // insert values to current entry
    list_curr->paddr = paddr;
    list_curr->count = count;
    list_curr->next = NULL;
    // connect previous entry with the current entry
    list_prev->next = list_curr;
    // prepare for the next iteration
    list_prev = list_curr;
    remaining_length -= count;
  }

  return list_head;
}

/*
* sg_destroy    Destroy a scatter-gather list
*
* @in sg_list   A scatter-gather list
*/
void sg_destroy(sg_entry_t *sg_list)
{
  while (sg_list != NULL)
  {
    sg_entry_t* next_sg_entry_t = sg_list->next;
    free(sg_list);
    sg_list = next_sg_entry_t;
  }
}

int main(int argc, char *argv[]) 
{

}
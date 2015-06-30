#include <stdlib.h>
#include "sg_copy.h"

/*
* init_entry    Initialize an entry in a scatter-gather list
*
* @in entry     Entry to initilaize
* @in paddr     Physical address of the initialized entry
* @in count     The number of bytes in the initialized entry
* @in next_entry Pointer to the next entry is the scatter-gather list
*/
void init_entry(sg_entry_t* entry, physaddr_t paddr, int count, sg_entry_t* next_entry)
{
  if (entry == NULL) return;

  entry->paddr = paddr;
  entry->count = count;
  entry->next = next_entry;
}

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
  
  // check for illegal input parameters
  if (buf == NULL) return NULL;
  if (length <= 0) return NULL;

  // align the entries on a PAGE_SIZE address, starting from the second entry.
  // hence, count how many bytes to include in the first entry in order
  // to align the rest of the entries
  while (((paddr + count) % PAGE_SIZE) && (count < length))
  {
    count++;
  }

  // check if the initial physical address is aligned on a PAGE_SIZE address 
  if (count == 0)
  {
    count = length <= PAGE_SIZE ? length : PAGE_SIZE;
  }

  // create new scatter-gather list
  list_head = (sg_entry_t*)malloc(sizeof(sg_entry_t));
  init_entry(list_head, paddr, count, NULL);
  list_prev = list_head;

  // update status
  remaining_length = length - count;

  while (remaining_length > 0)
  {
    // calculate count and address
    paddr += count;
    count = remaining_length <= PAGE_SIZE ?
            remaining_length : PAGE_SIZE;

    // create new entry
    list_curr = (sg_entry_t*)malloc(sizeof(sg_entry_t));
    // insert values to the current entry
    init_entry(list_curr, paddr, count, NULL);
    // connect previous entry with the current entry
    list_prev->next = list_curr;
    // prepare for the next iteration
    list_prev = list_curr;
    // update status
    remaining_length -= count;
  }

  return list_head;
}

/*
* sg_destroy    Destroy a scatter-gather list
*
* @in sg_list   A scatter-gather list
*/
void sg_destroy(sg_entry_t* sg_list)
{
  sg_entry_t* next_sg_entry_t;

  while (sg_list != NULL)
  {
    // avoid memory access violation.
    // either an uninitialized entry, or a list which a part of it was already
    // freed
    if ((sg_list->count <= 0) || 
        (sg_list->next != NULL && sg_list->next->count <= 0))
    {
      sg_list->next = NULL;
    }
    next_sg_entry_t = sg_list->next;
    free(sg_list);
    sg_list = next_sg_entry_t;
  }
}

/*
* sg_copy       Copy bytes using scatter-gather lists
*
* @in src       Source sg list
* @in dest      Destination sg list
* @in src_offset Offset into source
* @in count     Number of bytes to copy
*
* @ret          Actual number of bytes copied
*
* @note         The function copies "count" bytes from "src",
*               starting from "src_offset" into the beginning of "dest".
*               The scatter gather list can be of arbitrary length so it is
*               possible that fewer bytes can be copied.
*               The function returns the actual number of bytes copied
*/
int sg_copy(sg_entry_t* src, sg_entry_t* dest, int src_offset, int count)
{
  sg_entry_t* src_curr = src;
  sg_entry_t* dest_curr = dest;
  sg_entry_t* dest_prev;
  void* pointer;
  int bytes_copied = 0;
  int bytes_skipped = 0;
  int remaining_bytes_to_copy = count;
  int offset_in_first_entry;
  int remaining_bytes_in_src_entry;
  int bytes_to_copy_from_src_entry;

  // no bytes are copied if one of the parameters is illogical
  if (dest == NULL) return 0;
  if (src_offset < 0) return 0;
  if (count <= 0) return 0;

  // skip entries (and bytes) before the offset
  while ((src_curr != NULL) && ((bytes_skipped + src_curr->count) <= src_offset))
  {
    bytes_skipped += src_curr->count;
    src_curr = src_curr->next;
  }

  // check if the src exists and if the offset is smaller than the total number
  // of available bytes. if not, no bytes are copied
  if (src_curr == NULL) return 0;
  // src_curr now points to the entry from which the first byte is copied

  // calculate how many bytes to copy from the current source entry
  offset_in_first_entry = src_offset - bytes_skipped;
  remaining_bytes_in_src_entry = src_curr->count - offset_in_first_entry;
  bytes_to_copy_from_src_entry = 
    remaining_bytes_to_copy <= remaining_bytes_in_src_entry ?
    remaining_bytes_to_copy : remaining_bytes_in_src_entry;
  
  // copy from source to destination
  init_entry(dest_curr,
    src_curr->paddr + offset_in_first_entry,
    bytes_to_copy_from_src_entry,
    NULL);
  dest_prev = dest_curr;

  //update status
  bytes_copied += bytes_to_copy_from_src_entry;
  remaining_bytes_to_copy -= bytes_to_copy_from_src_entry;
  // move to next source entry
  src_curr = src_curr->next;

  while (remaining_bytes_to_copy > 0 && src_curr != NULL && src_curr->count > 0)
  {
    // calculate how many bytes to copy from the current source entry
    remaining_bytes_in_src_entry = src_curr->count;
    bytes_to_copy_from_src_entry = 
      remaining_bytes_to_copy <= remaining_bytes_in_src_entry ?
      remaining_bytes_to_copy : remaining_bytes_in_src_entry;

    // copy from source to destination
    pointer = phys_to_ptr(src_curr->paddr);
    dest_curr = sg_map(pointer, bytes_to_copy_from_src_entry);
    // connect previous entry with the current entry
    dest_prev->next = dest_curr;
    // prepare for the next iteration
    while (dest_curr->next != NULL)
    {
      dest_curr = dest_curr->next;
    }
    dest_prev = dest_curr;
    src_curr = src_curr->next;

    // update status
    bytes_copied += bytes_to_copy_from_src_entry;
    remaining_bytes_to_copy -= bytes_to_copy_from_src_entry;
  }

  return bytes_copied;
}

int main(int argc, char *argv[]) 
{
  // example
  int var;
  int var2;
  sg_entry_t* head = sg_map(&var, 74);
  sg_entry_t* next = sg_map(&var2, 47);

  sg_destroy(head->next);
  head->next = next;

  sg_entry_t* dst = (sg_entry_t*)malloc(sizeof(sg_entry_t));
  int bytes_copied = sg_copy(head, dst, 6, 84);

  sg_destroy(next);
  sg_destroy(head);
  sg_destroy(dst);

  return 1;
}

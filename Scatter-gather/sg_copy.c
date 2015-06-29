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
  
  if (buf == NULL) return NULL;
  if (length <= 0) return NULL;

  while (((paddr + count) % PAGE_SIZE) && (count < length))
  {
    count++;
  }

  // check if initial physical address is aligned on a PAGE_SIZE address 
  if (count == 0)
  {
    count = length <= PAGE_SIZE ? length : PAGE_SIZE;
  }

  list_head = (sg_entry_t*)malloc(sizeof(sg_entry_t));
  list_head->paddr = paddr;
  list_head->count = count;
  list_head->next = NULL;
  list_prev = list_head;

  remaining_length = length - count;

  while (remaining_length > 0)
  {
    // calculate count and address
    paddr += count;
    count = remaining_length <= PAGE_SIZE ?
            remaining_length : PAGE_SIZE;

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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
  sg_entry_t* src_curr = src;
  sg_entry_t* dest_curr = dest;
  sg_entry_t* dest_prev;
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

  // skip entries (and bytes) before the offset.
  while ((src_curr != NULL) && ((bytes_skipped + src_curr->count) <= src_offset))
  {
    bytes_skipped += src_curr->count;
    src_curr = src_curr->next;
  }

  // check if the src exists and if the offset is smaller than the total number
  // of available bytes. if not, 0 bytes are copied
  if (src_curr == NULL) return 0;

  // src_curr now points to the entry from which the first byte is copied
  offset_in_first_entry = src_offset - bytes_skipped;
  remaining_bytes_in_src_entry = src_curr->count - offset_in_first_entry;
  bytes_to_copy_from_src_entry = 
    remaining_bytes_to_copy <= remaining_bytes_in_src_entry ?
    remaining_bytes_to_copy : remaining_bytes_in_src_entry;
  
  // copy from source to destination
  dest_curr->paddr = src_curr->paddr + offset_in_first_entry;
  dest_curr->count = bytes_to_copy_from_src_entry;
  dest_curr->next = NULL;
  dest_prev = dest_curr;

  bytes_copied += bytes_to_copy_from_src_entry;
  remaining_bytes_to_copy -= bytes_to_copy_from_src_entry;
  src_curr = src_curr->next;

  while (remaining_bytes_to_copy > 0 && src_curr != NULL && src_curr->count > 0)
  {
    dest_curr = (sg_entry_t*)malloc(sizeof(sg_entry_t));
    remaining_bytes_in_src_entry = src_curr->count;
    bytes_to_copy_from_src_entry = 
      remaining_bytes_to_copy <= remaining_bytes_in_src_entry ?
      remaining_bytes_to_copy : remaining_bytes_in_src_entry;

    // copy from source to destination
    dest_curr->paddr = src_curr->paddr;
    dest_curr->count = bytes_to_copy_from_src_entry;
    dest_curr->next = NULL;
    dest_prev->next = dest_curr;
    dest_prev = dest_curr;

    bytes_copied += bytes_to_copy_from_src_entry;
    remaining_bytes_to_copy -= bytes_to_copy_from_src_entry;
    src_curr = src_curr->next;
  }

  return bytes_copied;
}

int main(int argc, char *argv[]) 
{
  // test
  int var;
  sg_entry_t* head = sg_map(&var, 14);
  sg_entry_t* dst = (sg_entry_t*)malloc(sizeof(sg_entry_t));
  int bytes_copied = sg_copy(head, dst, 0, 62);

  sg_destroy(head);
  sg_destroy(dst);

  return 1;
}

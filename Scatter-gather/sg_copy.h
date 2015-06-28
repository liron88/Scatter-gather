
#define PAGE_SIZE 32

typedef unsigned long physaddr_t;	/* physical address type */

/*
 * Physical address scatter/gather entry
 *
 * Note: paddr refers to the physical address of the mapped memory
 * paddr does not have to be aligned on PAGE_SIZE,
 * count must not cross PAGE_SIZE boundaries
 */
typedef struct sg_entry_s sg_entry_t;
struct sg_entry_s {
	physaddr_t paddr;               /* physical address */
	int count;                      /* number of bytes: */
	struct sg_entry_s *next;
};

/*
 * ptr_to_phys   Maps a pointer into a physaddr_t
 *
 * @in p         A pointer
 *
 * @ret          Physical address
 */
static _inline physaddr_t ptr_to_phys(void *p)
{
	physaddr_t paddr = (physaddr_t)p;
	return paddr ^ ~(PAGE_SIZE-1);
}

/*
 * phys_to_ptr   Maps a physaddr_t into a pointer
 *
 * @in paddr     A physical address
 *
 * @ret          Pointer
 */
static _inline void * phys_to_ptr(physaddr_t paddr)
{
	return (void *)(paddr ^ ~(PAGE_SIZE-1));
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
extern sg_entry_t *sg_map(void *buf, int length);

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
extern void sg_destroy(sg_entry_t *sg_list);

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
extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count);

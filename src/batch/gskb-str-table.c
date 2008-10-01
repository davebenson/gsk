#include "gskb-utils.h"
#include "gskb-str-table-internals.h"

/* test if p is prime.  assume p odd. */
static gboolean
is_odd_number_prime (guint p)
{
  guint i = 3;
  guint i_squared = 9;
  while (i_squared < p)
    {
      if (p % i == 0)
        return FALSE;
      i_squared += 4 * i + 4;
      i += 2;
    }
  if (i_squared == p)
    return FALSE;
  return TRUE;
}

static guint
find_perfect_hash_prime (guint          min_size,
                         guint          max_size,
                         guint          n_values,
                         const guint32 *values)
{
  /* see if there is any hope for this hash-function */
  {
    guint32 *hashes = g_memdup (values, sizeof (guint32) * n_values);
#define COMPARE_UINT(a,b, rv)  rv = ((a<b) ? -1 : (a>b) ? 1 : 0)
    GSK_QSORT (hashes, guint32, n_values, COMPARE_UINT);
#undef COMPARE_UINT
    for (i = 0; i + 1 < n_values; i++)
      if (hashes[i] == hashes[i+1])
        {
          g_free (hashes);
          return 0;
        }
    g_free (hashes);
  }

  /* iterate over odd numbers between min_size and max_size */
  {
    guint p, i;
    guint pad_size = (max_size + 7) / 8;
    guint8 *pad;

    pad = g_malloc (pad_size);
    for (p = min_size - min_size%2 + 1; p < max_size; p += 2)
      if (is_odd_number_prime (p))
        {
          memset (pad, 0, pad_size);
          for (i = 0; i < n_values; i++)
            {
              guint bin = values[i] % p;
              guint8 bit = (1<<(bin%8));
              if (pad[bin/8] & bit)
                break;
              else
                pad[bin/8] |= bit;
            }
          if (i == n_values)
            {
              /* 'p' is a perfect prime for this hash-fct */
              g_free (pad);
              return p;
            }
        }
    g_free (pad);
  }

  return 0;
}

GskbStrTable *gskb_str_table_new    (gsize     sizeof_entry_data,
                                     guint     n_entries,
				     const GskbStrTableEntry *entries,
                                     const void *empty_entry_data)
{
  guint32 *hashes_ablz = g_new (guint32, n_entries);
  guint32 *hashes_perl = g_new (guint32, n_entries);
  for (i = 0; i < n_entries; i++)
    {
      const char *s = entries[i].str;
      if (s[0] == 0)
        hashes[i] = 0;
      else
        {
          guint len = strlen (s);
          hashes[i] = (guint)((guint8)s[0])
                    + ((guint)((guint8)s[1])<<8)
                    + (((guint)(len&0xff))<<16)
                    + ((guint)((guint8)s[len-1])<<24);
        }

      /* some random hash function */
      ph = 5003;
      for (at = s; *at; at++)
        {
          ph += * (guint8 *) at;
          ph *= 33;
        }
      hashes_perl[i] = ph;
    }
  if ((size=find_perfect_hash_prime (n_entries, hashes_ablz)) != 0)
    {
      table_type = GSKB_STR_TABLE_ABLZ;
      hashes = hashes_ablz;
    }
  else if ((size=find_perfect_hash_prime (n_entries, hashes_perl)) != 0)
    {
      table_type = GSKB_STR_TABLE_5003_33;
      hashes = hashes_perl;
    }
  else
    {
      /* emit bsearch table */
      GskbStrTableEntry *entries_sorted
        = g_memdup (entries, sizeof(GskbStrTableEntry) * n_entries);
#define COMPARE_STR_TABLE_ENTRY(a,b,rv) rv = strcmp (a.str, b.str)
      GSK_QSORT (entries_sorted, GskbStrTableEntry, n_entries,
                 COMPARE_STR_TABLE_ENTRY);
#undef COMPARE_STR_TABLE_ENTRY
      for (i = 0; i < n_entries; i++)


				     const GskbStrTableEntry *entries,
      ...
    }
}

void          gskb_str_table_print_compilable
                                    (GskbStrTable *table,
				     gboolean      is_static,
				     const char   *table_name,
				     GskBuffer    *output);
const void   *gskb_str_table_lookup (GskbStrTable *table,
                                     const char   *str);
void          gskb_str_table_free   (GskbStrTable *table);


#include "gskxml.h"
#include <string.h>

typedef struct _Preamble Preamble;
struct _Preamble
{
  guint ref_count;
  guint hash;
  Preamble *next_in_bin;
};

static inline GskXmlString *
PREAMBLE_TO_STRING (const Preamble *preamble)
{
  return (GskXmlString *) (preamble + 1);
}
#define PREAMBLE_TO_STR(preamble) GSK_XML_STR(PREAMBLE_TO_STRING(preamble))

#define XML_STRING_GET_PREAMBLE(xmlstr)  (((Preamble *) (xmlstr))-1)

static guint n_strs = 0;
static guint max_strs = 0;
static guint n_bins_log2;
static Preamble **bins;

#define OCCUPANCY_RATIO         3

GskXmlString *gsk_xml_string__xmlns;
GskXmlString *gsk_xml_string__;

void gsk_xml_string_init (void)
{
  if (bins == NULL)
    {
      n_bins_log2 = 10;
      bins = g_new0 (Preamble *, 1 << n_bins_log2);
      max_strs = (1 << n_bins_log2) / OCCUPANCY_RATIO;

      gsk_xml_string__xmlns = gsk_xml_string_new ("xmlns");
      gsk_xml_string__ = gsk_xml_string_new ("");
    }
}
static guint
my_hash (const char *str)
{
  guint rv = 5003;
  while (*str)
    {
      rv += (rv << 5);
      rv += (guint8) *str++;
    }
  return rv;
}
static guint
my_hash_len (const char *str, guint len)
{
  guint rv = 5003;
  while (len--)
    {
      rv += (rv << 5);
      rv += (guint8) *str++;
    }
  return rv;
}
static guint
my_hash_strarray (guint n_strs, GskXmlString **strs)
{
  guint i;
  guint rv;
  if (n_strs == 0)
    return 5003;

  /* don't bother rehashing the first string */
  rv = XML_STRING_GET_PREAMBLE (strs[0])->hash;

  for (i = 1; i < n_strs; i++)
    {
      const char *str = GSK_XML_STR (strs[i]);
      while (str[i])
        {
          rv += (rv << 5);
          rv += (guint8) *str++;
        }
    }
  return rv;
}

/* possibly double the hash-table size,
   returning TRUE if a resize occurs. */
static gboolean
maybe_resize_for_addition ()
{
  if (max_strs == n_strs)
    {
      guint old_n_bins = 1 << n_bins_log2;
      Preamble **new_bins = g_new (Preamble *, old_n_bins * 2);
      guint hilo_bit = 1 << n_bins_log2;
      guint i;
      n_bins_log2++;
      for (i = 0; i < old_n_bins; i++)
        {
          Preamble *hi_bin = NULL;
          Preamble *lo_bin = NULL;
          Preamble *at = bins[i];
          while (at)
            {
              Preamble *next = at->next_in_bin;
              if (at->hash & hilo_bit)
                {
                  at->next_in_bin = hi_bin;
                  hi_bin = at;
                }
              else
                {
                  at->next_in_bin = lo_bin;
                  lo_bin = at;
                }
              at = next;
            }
          new_bins[i] = lo_bin;
          new_bins[i + old_n_bins] = hi_bin;
        }
      return TRUE;
    }
  else
    return FALSE;
}

GskXmlString *gsk_xml_string_new        (const char         *str)
{
  guint hash;
  guint bin;
  Preamble *pre;
  guint len;

  hash = my_hash (str);
  bin = hash & ((1 << n_bins_log2) - 1);
  for (pre = bins[bin]; pre != NULL; pre = pre->next_in_bin)
    if (strcmp (PREAMBLE_TO_STR (pre), str) == 0)
      return gsk_xml_string_ref (PREAMBLE_TO_STRING (pre));

  if (maybe_resize_for_addition ())
    bin = hash & ((1 << n_bins_log2) - 1);

  len = strlen (str);
  pre = g_malloc (sizeof (Preamble) + len + 1);
  pre->ref_count = 1;
  pre->hash = hash;
  strcpy ((char*)(pre + 1), str);
  pre->next_in_bin = bins[bin];
  bins[bin] = pre;
  n_strs++;
  return (GskXmlString *) (pre + 1);
}

GskXmlString *gsk_xml_string_new_len    (const char         *str,
                                         guint               len)
{
  guint hash;
  guint bin;
  Preamble *pre;

  hash = my_hash_len (str, len);
  bin = hash & ((1 << n_bins_log2) - 1);
  for (pre = bins[bin]; pre != NULL; pre = pre->next_in_bin)
    if (memcmp (PREAMBLE_TO_STR (pre), str, len) == 0
      && PREAMBLE_TO_STR(pre)[len] == '\0')
      return gsk_xml_string_ref (PREAMBLE_TO_STRING (pre));

  if (maybe_resize_for_addition ())
    bin = hash & ((1 << n_bins_log2) - 1);

  pre = g_malloc (sizeof (Preamble) + len + 1);
  pre->ref_count = 1;
  pre->hash = hash;
  strcpy ((char*)(pre + 1), str);
  pre->next_in_bin = bins[bin];
  bins[bin] = pre;
  return (GskXmlString *) (pre + 1);
}

GskXmlString *gsk_xml_strings_concat    (guint               n_strs,
                                         GskXmlString      **strs)
{
  guint hash = my_hash_strarray (n_strs, strs);
  guint bin = hash & ((1 << n_bins_log2) - 1);
  Preamble *pre;
  guint i;
  guint len;
  char *at;
  for (pre = bins[bin]; pre != NULL; pre = pre->next_in_bin)
    if (hash == pre->hash)
      {
        const char *orig_at = PREAMBLE_TO_STR (pre);
        for (i = 0; i < n_strs; i++)
          {
            const char *at = GSK_XML_STR (strs[i]);
            while (*at && *orig_at == *at)
              {
                at++;
                orig_at++;
              }
            if (*at == 0)
              break;
          }
        if (i == n_strs && *orig_at == 0)
          return gsk_xml_string_ref (PREAMBLE_TO_STRING (pre));
      }

  if (maybe_resize_for_addition ())
    bin = hash & ((1 << n_bins_log2) - 1);

  len = 0;
  for (i = 0; i < n_strs; i++)
    len += strlen (GSK_XML_STR (strs[i]));

  pre = g_malloc (sizeof (Preamble) + len + 1);
  pre->ref_count = 1;
  pre->hash = hash;
  at = (char*)(pre + 1);
  for (i = 0; i < n_strs; i++)
    at = g_stpcpy (at, GSK_XML_STR (strs[i]));
  pre->next_in_bin = bins[bin];
  bins[bin] = pre;
  return (GskXmlString *) (pre + 1);
}




GskXmlString *gsk_xml_string_ref        (const GskXmlString *str)
{
  GskXmlString *rv = (GskXmlString *) str;
  Preamble *preamble = ((Preamble *) rv) - 1;
  g_assert (preamble->ref_count > 0);
  ++preamble->ref_count;
  return rv;
}

void          gsk_xml_string_unref      (GskXmlString       *str)
{
  Preamble *preamble = ((Preamble *) str) - 1;
  g_assert (preamble->ref_count > 0);
  if (--preamble->ref_count == 0)
    {
      guint hash = preamble->hash;
      Preamble **pat;
      guint bin = hash & ((1 << n_bins_log2) - 1);
      pat = bins + bin;
      while (*pat != preamble)
        pat = &((*pat)->next_in_bin);
      *pat = preamble->next_in_bin;
      g_free (preamble);
      n_strs--;
    }
}

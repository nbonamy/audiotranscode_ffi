
#include "FlacMetadata.h"
#include <FLAC/all.h>

using namespace std;

static FLAC__bool populate_seekpoint_values(const char *filename, FLAC__StreamMetadata *block);
static FLAC__bool add_seekpoints(const char *filename, FLAC__Metadata_Chain *chain, const char *specification);
static FLAC__bool grabbag__seektable_convert_specification_to_template(const char *spec, FLAC__bool only_explicit_placeholders, FLAC__uint64 total_samples_to_encode, uint32_t sample_rate, FLAC__StreamMetadata *seektable_template, FLAC__bool *spec_has_real_points);

bool FlacMetadata::addSeekTable(const string& filename)
{
  // open metadata chain
  FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
  if (!FLAC__metadata_chain_read(chain, filename.c_str())) {
    return false;
  }
  
  // do it
  add_seekpoints(filename.c_str(), chain, "1s;");
  
  // save
  FLAC__metadata_chain_sort_padding(chain);
  FLAC__metadata_chain_write(chain, true, true);
  
  // clean-up
  FLAC__metadata_chain_delete(chain);
  
  // success!
  return true;
}

FLAC__bool add_seekpoints(const char *filename, FLAC__Metadata_Chain *chain, const char *specification)
{
  FLAC__bool ok = true, found_seektable_block = false;
  FLAC__StreamMetadata *block = 0;
  FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
  FLAC__uint64 total_samples = 0;
  unsigned sample_rate = 0;
  
  if(0 == iterator) {
    return false;
  }
  
  FLAC__metadata_iterator_init(iterator, chain);
  
  do {
    block = FLAC__metadata_iterator_get_block(iterator);
    if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
      sample_rate = block->data.stream_info.sample_rate;
      total_samples = block->data.stream_info.total_samples;
    }
    else if(block->type == FLAC__METADATA_TYPE_SEEKTABLE)
      found_seektable_block = true;
  } while(!found_seektable_block && FLAC__metadata_iterator_next(iterator));
  
  if(total_samples == 0) {
    //flac_fprintf(stderr, "%s: ERROR: cannot add seekpoints because STREAMINFO block does not specify total_samples\n", filename);
    FLAC__metadata_iterator_delete(iterator);
    return false;
  }
  
  if(!found_seektable_block) {
    /* create a new block */
    block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
    if(0 == block) {
      //die("out of memory allocating SEEKTABLE block");
      return false;
    }
    while(FLAC__metadata_iterator_prev(iterator))
      ;
    if(!FLAC__metadata_iterator_insert_block_after(iterator, block)) {
      //print_error_with_chain_status(chain, "%s: ERROR: adding new SEEKTABLE block to metadata", filename);
      FLAC__metadata_object_delete(block);
      FLAC__metadata_iterator_delete(iterator);
      return false;
    }
    /* iterator is left pointing to new block */
    FLAC__ASSERT(FLAC__metadata_iterator_get_block(iterator) == block);
  }
  
  FLAC__metadata_iterator_delete(iterator);
  
  FLAC__ASSERT(0 != block);
  FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_SEEKTABLE);
  
  if(!grabbag__seektable_convert_specification_to_template(specification, /*only_explicit_placeholders=*/false, total_samples, sample_rate, block, /*spec_has_real_points=*/0)) {
    //flac_fprintf(stderr, "%s: ERROR (internal) preparing seektable with seekpoints\n", filename);
    return false;
  }
  
  ok = populate_seekpoint_values(filename, block);
  
  if(ok)
    (void) FLAC__format_seektable_sort(&block->data.seek_table);
  
  return ok;
}

/*
 * local routines
 */

typedef struct {
  FLAC__StreamMetadata_SeekTable *seektable_template;
  FLAC__uint64 samples_written;
  FLAC__uint64 audio_offset, last_offset;
  unsigned first_seekpoint_to_check;
  FLAC__bool error_occurred;
  FLAC__StreamDecoderErrorStatus error_status;
} ClientData;

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
  ClientData *cd = (ClientData*)client_data;
  
  (void)buffer;
  FLAC__ASSERT(0 != cd);
  
  if(!cd->error_occurred) {
    const unsigned blocksize = frame->header.blocksize;
    const FLAC__uint64 frame_first_sample = cd->samples_written;
    const FLAC__uint64 frame_last_sample = frame_first_sample + (FLAC__uint64)blocksize - 1;
    FLAC__uint64 test_sample;
    unsigned i;
    for(i = cd->first_seekpoint_to_check; i < cd->seektable_template->num_points; i++) {
      test_sample = cd->seektable_template->points[i].sample_number;
      if(test_sample > frame_last_sample) {
        break;
      }
      else if(test_sample >= frame_first_sample) {
        cd->seektable_template->points[i].sample_number = frame_first_sample;
        cd->seektable_template->points[i].stream_offset = cd->last_offset - cd->audio_offset;
        cd->seektable_template->points[i].frame_samples = blocksize;
        cd->first_seekpoint_to_check++;
        /* DO NOT: "break;" and here's why:
         * The seektable template may contain more than one target
         * sample for any given frame; we will keep looping, generating
         * duplicate seekpoints for them, and we'll clean it up later,
         * just before writing the seektable back to the metadata.
         */
      }
      else {
        cd->first_seekpoint_to_check++;
      }
    }
    cd->samples_written += blocksize;
    if(!FLAC__stream_decoder_get_decode_position(decoder, &cd->last_offset))
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }
  else
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
  ClientData *cd = (ClientData*)client_data;
  
  (void)decoder;
  FLAC__ASSERT(0 != cd);
  
  if(!cd->error_occurred) { /* don't let multiple errors overwrite the first one */
    cd->error_occurred = true;
    cd->error_status = status;
  }
}

FLAC__bool populate_seekpoint_values(const char *filename, FLAC__StreamMetadata *block)
{
  FLAC__StreamDecoder *decoder;
  ClientData client_data;
  FLAC__bool ok = true;
  
  FLAC__ASSERT(0 != block);
  FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_SEEKTABLE);
  
  client_data.seektable_template = &block->data.seek_table;
  client_data.samples_written = 0;
  /* client_data.audio_offset must be determined later */
  client_data.first_seekpoint_to_check = 0;
  client_data.error_occurred = false;
  
  decoder = FLAC__stream_decoder_new();
  
  if(0 == decoder) {
    //flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) creating the decoder instance\n", filename);
    return false;
  }
  
  FLAC__stream_decoder_set_md5_checking(decoder, false);
  FLAC__stream_decoder_set_metadata_ignore_all(decoder);
  
  if(FLAC__stream_decoder_init_file(decoder, filename, write_callback_, /*metadata_callback=*/0, error_callback_, &client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
//    flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) initializing the decoder instance (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
    ok = false;
  }
  
  if(ok && !FLAC__stream_decoder_process_until_end_of_metadata(decoder)) {
//    flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
    ok = false;
  }
  
  if(ok && !FLAC__stream_decoder_get_decode_position(decoder, &client_data.audio_offset)) {
//    flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file\n", filename);
    ok = false;
  }
  client_data.last_offset = client_data.audio_offset;
  
  if(ok && !FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
//    flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
    ok = false;
  }
  
  if(ok && client_data.error_occurred) {
//    flac_fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%u:%s)\n", filename, (unsigned)client_data.error_status, FLAC__StreamDecoderErrorStatusString[client_data.error_status]);
    ok = false;
  }
  
  //*needs_write = true;
  FLAC__stream_decoder_delete(decoder);
  return ok;
}

FLAC__bool grabbag__seektable_convert_specification_to_template(const char *spec, FLAC__bool only_explicit_placeholders, FLAC__uint64 total_samples_to_encode, uint32_t sample_rate, FLAC__StreamMetadata *seektable_template, FLAC__bool *spec_has_real_points)
{
  uint32_t i;
  const char *pt;
  
  FLAC__ASSERT(0 != spec);
  FLAC__ASSERT(0 != seektable_template);
  FLAC__ASSERT(seektable_template->type == FLAC__METADATA_TYPE_SEEKTABLE);
  
  if(0 != spec_has_real_points)
    *spec_has_real_points = false;
  
  for(pt = spec, i = 0; pt && *pt; i++) {
    const char *q = strchr(pt, ';');
    FLAC__ASSERT(0 != q);
    
    if(q > pt) {
      if(0 == strncmp(pt, "X;", 2)) { /* -S X */
        if(!FLAC__metadata_object_seektable_template_append_placeholders(seektable_template, 1))
          return false;
      }
      else if(q[-1] == 'x') { /* -S #x */
        if(total_samples_to_encode > 0) { /* we can only do these if we know the number of samples to encode up front */
          if(0 != spec_has_real_points)
            *spec_has_real_points = true;
          if(!only_explicit_placeholders) {
            const int n = (uint32_t)atoi(pt);
            if(n > 0)
              if(!FLAC__metadata_object_seektable_template_append_spaced_points(seektable_template, (uint32_t)n, total_samples_to_encode))
                return false;
          }
        }
      }
      else if(q[-1] == 's') { /* -S #s */
        if(total_samples_to_encode > 0 && sample_rate > 0) { /* we can only do these if we know the number of samples and sample rate to encode up front */
          if(0 != spec_has_real_points)
            *spec_has_real_points = true;
          if(!only_explicit_placeholders) {
            const double sec = atof(pt);
            if(sec > 0.0) {
              uint32_t samples = (uint32_t)(sec * (double)sample_rate);
              /* Restrict seekpoints to two per second of audio. */
              samples = samples < sample_rate / 2 ? sample_rate / 2 : samples;
              if(samples > 0) {
                /* +1 for the initial point at sample 0 */
                if(!FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(seektable_template, samples, total_samples_to_encode))
                  return false;
              }
            }
          }
        }
      }
      else { /* -S # */
        if(0 != spec_has_real_points)
          *spec_has_real_points = true;
        if(!only_explicit_placeholders) {
          char *endptr;
          const FLAC__int64 n = (FLAC__int64)strtoll(pt, &endptr, 10);
          if(
             (n > 0 || (endptr > pt && *endptr == ';')) && /* is a valid number (extra check needed for "0") */
             (total_samples_to_encode == 0 || (FLAC__uint64)n < total_samples_to_encode) /* number is not >= the known total_samples_to_encode */
             )
            if(!FLAC__metadata_object_seektable_template_append_point(seektable_template, (FLAC__uint64)n))
              return false;
        }
      }
    }
    
    pt = ++q;
  }
  
  if(!FLAC__metadata_object_seektable_template_sort(seektable_template, /*compact=*/true))
    return false;
  
  return true;
}

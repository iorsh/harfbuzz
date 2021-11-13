/*
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "hb.hh"

#include "hb-justification.h"
#include "hb-buffer.hh"

void
hb_squeeze_justified_buffer(hb_buffer_t *buffer)
{
    unsigned int i_in = 0u, i_out = 0u;

    while(i_in < buffer->len)
    {
        if (i_in != i_out) {
            buffer->info[i_out] = buffer->info[i_in];
            buffer->pos[i_out] = buffer->pos[i_in];
        }

        if(buffer->info[i_in].codepoint == LINE_BREAK)
        {
            while(buffer->info[i_in + 1].codepoint == LINE_BREAK)
            {
                ++i_in;
            }
        }

        ++i_out;
        ++i_in;
    }

    buffer->len = i_out;
}

void
hb_split_buffer_to_lines(hb_buffer_t           *buffer,
                         unsigned int           num_target_lengths,
                         const hb_position_t   *target_lengths,
                         hb_codepoint_t         space)
{
    assert(hb_buffer_has_positions(buffer));
    assert(num_target_lengths > 0u);

    unsigned int current_subbuffer_idx = 0u;

    /* Index of the first subbuffer glyph in the buffer */
    unsigned int current_subbuffer_start = 0u; 

    /* Latest breaking position in the buffer (index of first glyph after the break) */
    unsigned int break_candidate = 0u; /* Initialize to invalid */

    hb_position_t current_target_length = target_lengths[current_subbuffer_idx];
    hb_position_t subbuffer_length = 0;

    assert(current_target_length > 0);

    /* Split buffer into subbuffers in a greedy manner */
    for (unsigned int i = 0u; i < buffer->len; ++i) {
        subbuffer_length += buffer->pos[i].x_advance;

        /* TODO(iorsh): Properly check for word boundary*/
        if (buffer->info[i].codepoint == space){
            /* Skip leading spaces */
            if (i == current_subbuffer_start + 1) {
                buffer->info[i].codepoint = LINE_BREAK;
                buffer->pos[i].x_advance = 0;
                current_subbuffer_start = i;
                subbuffer_length = 0;
            }

            /* For multiple spaces, keep the break candidate at the first one */
            if (i > 0 && buffer->info[i-1].codepoint != space) {
                break_candidate = i;
            }
        }

        bool collect_subbuffer = (subbuffer_length > current_target_length) &&
                                 (break_candidate > 0u);

        /* Force collecting last subbuffer when we reach the end */
        bool last_subbuffer = (i == buffer->len - 1) && (subbuffer_length > 0);
        if (last_subbuffer && !collect_subbuffer) {
            break_candidate = buffer->len;
            collect_subbuffer= true;
        }

        if (!collect_subbuffer) {
            continue;
        }

        /* We shall break the buffer at the latest breaking position */
        if (break_candidate < buffer->len) {
            buffer->info[break_candidate].codepoint = LINE_BREAK;
            buffer->pos[break_candidate].x_advance = 0;
        }

        ++current_subbuffer_idx;
        if (current_subbuffer_idx < num_target_lengths) {
            current_target_length = target_lengths[current_subbuffer_idx];
        } else {
            current_target_length = target_lengths[num_target_lengths - 1];
        }

        current_subbuffer_start = break_candidate;
        break_candidate = 0u; /* Initialize to invalid */

        /* Reset subbuffer length and go back to the beginning of
        the subbuffer. The loop increment will skip the leading space*/
        subbuffer_length = 0;
        i = current_subbuffer_start;
    }

    /* Squeeze buffer by purging */
    hb_squeeze_justified_buffer(buffer);
}

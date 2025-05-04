#ifndef KOUTIL_TERM_COLOR16_H
#define KOUTIL_TERM_COLOR16_H

#include "koutil/term/color.h"

namespace koutil::term::color {

constexpr auto black         = color_t::from_id(30); /**< Black color */
constexpr auto red           = color_t::from_id(31); /**< Red color */
constexpr auto green         = color_t::from_id(32); /**< Green color */
constexpr auto yellow        = color_t::from_id(33); /**< Yellow color */
constexpr auto blue          = color_t::from_id(34); /**< Blue color */
constexpr auto magenta       = color_t::from_id(35); /**< Magenta color */
constexpr auto cyan          = color_t::from_id(36); /**< Cyan color */
constexpr auto white         = color_t::from_id(37); /**< White color */
constexpr auto default_color = color_t::from_id(39); /**< Default color */

constexpr auto black_bright   = color_t::from_id(90); /**< Bright black color */
constexpr auto red_bright     = color_t::from_id(91); /**< Bright red color */
constexpr auto green_bright   = color_t::from_id(92); /**< Bright green color */
constexpr auto yellow_bright  = color_t::from_id(93); /**< Bright yellow color */
constexpr auto blue_bright    = color_t::from_id(94); /**< Bright blue color */
constexpr auto magenta_bright = color_t::from_id(95); /**< Bright magenta color */
constexpr auto cyan_bright    = color_t::from_id(96); /**< Bright cyan color */
constexpr auto white_bright   = color_t::from_id(97); /**< Bright white color */

}

#endif

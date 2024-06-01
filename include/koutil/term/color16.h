#ifndef KOUTIL_TERM_COLOR16_H
#define KOUTIL_TERM_COLOR16_H

#include "koutil/term/color.h"

namespace koutil::term::color {

constexpr auto black         = Color::from_id(30); /**< Black color */
constexpr auto red           = Color::from_id(31); /**< Red color */
constexpr auto green         = Color::from_id(32); /**< Green color */
constexpr auto yellow        = Color::from_id(33); /**< Yellow color */
constexpr auto blue          = Color::from_id(34); /**< Blue color */
constexpr auto magenta       = Color::from_id(35); /**< Magenta color */
constexpr auto cyan          = Color::from_id(36); /**< Cyan color */
constexpr auto white         = Color::from_id(37); /**< White color */
constexpr auto default_color = Color::from_id(39); /**< Default color */

constexpr auto black_bright   = Color::from_id(90); /**< Bright black color */
constexpr auto red_bright     = Color::from_id(91); /**< Bright red color */
constexpr auto green_bright   = Color::from_id(92); /**< Bright green color */
constexpr auto yellow_bright  = Color::from_id(93); /**< Bright yellow color */
constexpr auto blue_bright    = Color::from_id(94); /**< Bright blue color */
constexpr auto magenta_bright = Color::from_id(95); /**< Bright magenta color */
constexpr auto cyan_bright    = Color::from_id(96); /**< Bright cyan color */
constexpr auto white_bright   = Color::from_id(97); /**< Bright white color */

}

#endif

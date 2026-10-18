#ifndef JC_CONFIG_H
#define JC_CONFIG_H
/* C99 port of open-scrantic engine/config.rs (MIT, Glauber Lima).
 * See licenses/open-scrantic-MIT.txt for the permission notice. */
#include "engine.h"
/* UTF-8 INI with ASCII section/keys. Missing keys retain original defaults.
 * Invalid input returns false and writes defaults, never a partial config. */
bool jc_config_parse(const char *text, JCConfig *out);
/* Missing path succeeds with defaults. Other read/format failures return false
 * with defaults and an error. Does not create files. */
bool jc_config_load(const char *path, JCConfig *out, JCError *error);
/* Writes original INI keys. intro_saved takes precedence over intro.
 * Temporary file in the destination directory, then atomic POSIX rename. */
bool jc_config_save(const char *path, const JCConfig *config, JCError *error);
#endif

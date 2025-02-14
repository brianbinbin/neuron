/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <jwt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "persist/persist.h"
#include "user.h"
#include "utils/asprintf.h"
#include "utils/log.h"

#include "argparse.h"
#include "parser/neu_json_login.h"
#include "json/neu_json_fn.h"

#include "handle.h"
#include "utils/http.h"
#include "utils/neu_jwt.h"

#include "normal_handle.h"

void handle_ping(nng_aio *aio)
{
    neu_http_ok(aio, "{}");
}

void handle_login(nng_aio *aio)
{
    NEU_PROCESS_HTTP_REQUEST(
        aio, neu_json_login_req_t, neu_json_decode_login_req, {
            neu_json_login_resp_t login_resp = { 0 };
            neu_user_t *          user       = neu_load_user(req->name);

            if (NULL == user) {
                nlog_warn("could not find user `%s`", req->name);
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_INVALID_USER_OR_PASSWORD, {
                    neu_http_response(aio, error_code.error, result_error);
                });
            } else if (neu_user_check_password(user, req->pass)) {

                char *token  = NULL;
                char *result = NULL;

                int ret = neu_jwt_new(&token);
                if (ret != 0) {
                    NEU_JSON_RESPONSE_ERROR(NEU_ERR_NEED_TOKEN, {
                        neu_http_response(aio, error_code.error, result_error);
                        jwt_free_str(token);
                    });
                }

                login_resp.token = token;

                neu_json_encode_by_fn(&login_resp, neu_json_encode_login_resp,
                                      &result);
                neu_http_ok(aio, result);
                jwt_free_str(token);
                free(result);
            } else {
                nlog_warn("user `%s` password check fail", req->name);
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_INVALID_USER_OR_PASSWORD, {
                    neu_http_response(aio, error_code.error, result_error);
                });
            }

            neu_user_free(user);
        })
}

void handle_password(nng_aio *aio)
{
    NEU_PROCESS_HTTP_REQUEST_VALIDATE_JWT(
        aio, neu_json_password_req_t, neu_json_decode_password_req, {
            neu_user_t *user     = NULL;
            int         rv       = 0;
            int         pass_len = strlen(req->new_pass);

            if (pass_len < NEU_USER_PASSWORD_MIN_LEN ||
                pass_len > NEU_USER_PASSWORD_MAX_LEN) {
                nlog_error("user `%s` new password too short or too long",
                           req->name);
                rv = NEU_ERR_INVALID_PASSWORD_LEN;
            } else if (NULL == (user = neu_load_user(req->name))) {
                nlog_error("could not find user `%s`", req->name);
                rv = NEU_ERR_INVALID_USER_OR_PASSWORD;
            } else if (!neu_user_check_password(user, req->old_pass)) {
                nlog_error("user `%s` password check fail", req->name);
                rv = NEU_ERR_INVALID_USER_OR_PASSWORD;
            } else if (0 !=
                       (rv = neu_user_update_password(user, req->new_pass))) {
                nlog_error("user `%s` update password fail", req->name);
            } else if (0 != (rv = neu_save_user(user))) {
                nlog_error("user `%s` persist fail", req->name);
            }

            NEU_JSON_RESPONSE_ERROR(rv, {
                neu_http_response(aio, error_code.error, result_error);
            });

            neu_user_free(user);
        })
}

static char *file_string_read(size_t *length, const char *const path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        errno   = 0;
        *length = 0;
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *length = (size_t) ftell(fp);
    if (0 == *length) {
        fclose(fp);
        return NULL;
    }

    char *data = NULL;
    data       = (char *) malloc((*length + 1) * sizeof(char));
    if (NULL != data) {
        data[*length] = '\0';
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(data, sizeof(char), *length, fp);
        if (read != *length) {
            *length = 0;
            free(data);
            data = NULL;
        }
    } else {
        *length = 0;
    }

    fclose(fp);
    return data;
}

void handle_get_plugin_schema(nng_aio *aio)
{
    size_t len         = 0;
    char * schema_path = NULL;

    NEU_VALIDATE_JWT(aio);

    const char *schema_name = neu_http_get_param(aio, "schema_name", &len);
    if (schema_name == NULL || len == 0) {
        neu_http_bad_request(aio, "{\"error\": 1002}");
        return;
    }

    if (0 > neu_asprintf(&schema_path, "%s/schema/%s.json", g_plugin_dir,
                         schema_name)) {
        NEU_JSON_RESPONSE_ERROR(NEU_ERR_EINTERNAL, {
            neu_http_response(aio, error_code.error, result_error);
        });
        return;
    }

    char *buf = NULL;
    buf       = file_string_read(&len, schema_path);
    if (NULL == buf) {
        nlog_info("open %s error: %d", schema_path, errno);
        neu_http_not_found(aio, "{\"status\": \"error\"}");
        free(schema_path);
        return;
    }

    neu_http_ok(aio, buf);
    free(buf);
    free(schema_path);
}

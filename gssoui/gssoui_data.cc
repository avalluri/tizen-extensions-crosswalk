// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gssoui_data.h"

#include <string>

#include "gssoui/gssoui_logs.h"

GssouiData::GssouiData(GVariant* data)
    : map_(g_hash_table_new_full(g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)g_variant_unref)) {
  if (!data)
    return;

  gchar* key = NULL;
  GVariant* value = NULL;
  GVariantIter* iter = NULL;

  g_variant_get(data, "a{sv}", &iter);
  while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
    g_hash_table_insert(map_, g_strdup(key), g_variant_ref_sink(value));
  }
}

GssouiData::~GssouiData() {
  g_hash_table_unref(map_);
}

GVariant* GssouiData::ToVariant() const {
  GVariantBuilder builder;
  GHashTableIter iter;
  gchar* key = NULL;
  GVariant* value = NULL;

  g_variant_builder_init(&builder, (GVariantType*)"a{sv}");

  g_hash_table_iter_init(&iter, map_);
  while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value)) {
    g_variant_builder_add(&builder, "{sv}", key, g_variant_ref_sink(value));
  }

  return g_variant_builder_end(&builder);
}

const gchar* GssouiData::GetString(const char* key) const {
  GVariant* value = (GVariant*)g_hash_table_lookup(map_, key);
  if (value && g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
    return g_variant_get_string(value, NULL);
  
  return NULL;
}

bool GssouiData::GetBool(const char* key) const {
  GVariant* value = (GVariant*)g_hash_table_lookup(map_, key);
    
  return value && g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN) &&
         g_variant_get_boolean (value);
}

bool GssouiData::HasKey(const char* key) const {
  return g_hash_table_lookup(map_, key) != NULL;
}

void GssouiData::Add(const char* key, GVariant *value) {
  if (!key || !value)
    return;

  g_variant_ref_sink(value);  
  g_hash_table_insert(map_, reinterpret_cast<gpointer>(g_strdup(key)),
                      reinterpret_cast<gpointer>(value));
}

#ifndef NDEBUG
static void _dump(gpointer key, gpointer value, gpointer userdata) {
  std::string* buffer = reinterpret_cast<std::string*>(userdata);
  const char* k = reinterpret_cast<const char*>(key);
  GVariant* v = reinterpret_cast<GVariant*>(value);
  char* v_str = g_variant_print(v, TRUE);

  buffer->append(k).append(":").append(v_str).append(";");
  g_free(v_str);
}

void GssouiData::Dump(const char* msg) const {
  std::string str;
  str.push_back('{');
  g_hash_table_foreach(map_, _dump, &str);
  str.push_back('}');
  DBG("%s: \n %s", msg, str.c_str());
}
#endif  // NDEBUG

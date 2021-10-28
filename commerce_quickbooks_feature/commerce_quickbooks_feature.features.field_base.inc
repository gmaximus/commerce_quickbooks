<?php

/**
 * @file
 * commerce_quickbooks_feature.features.field_base.inc
 */

/**
 * Implements hook_field_default_field_bases().
 */
function commerce_quickbooks_feature_field_default_field_bases() {
  $field_bases = array();

  // Exported field_base: 'field_product_qbo_income_account'.
  $field_bases['field_product_qbo_income_account'] = array(
    'active' => 1,
    'cardinality' => 1,
    'deleted' => 0,
    'entity_types' => array(),
    'field_name' => 'field_product_qbo_income_account',
    'indexes' => array(
      'value' => array(
        0 => 'value',
      ),
    ),
    'locked' => 0,
    'module' => 'list',
    'settings' => array(
      'allowed_values' => array(
        1000000000 => 'Do Not Use - Placeholder',
      ),
      'allowed_values_function' => 'commerce_quickbooks_fetch_accounts',
    ),
    'translatable' => 0,
    'type' => 'list_text',
  );

  // Exported field_base: 'field_qbo_id'.
  $field_bases['field_qbo_id'] = array(
    'active' => 1,
    'cardinality' => 1,
    'deleted' => 0,
    'entity_types' => array(),
    'field_name' => 'field_qbo_id',
    'indexes' => array(
      'format' => array(
        0 => 'format',
      ),
    ),
    'locked' => 0,
    'module' => 'text',
    'settings' => array(
      'field_encrypt' => array(
        'encrypt' => 0,
      ),
      'max_length' => 255,
    ),
    'translatable' => 0,
    'type' => 'text',
  );

  // Exported field_base: 'field_qbo_status'.
  $field_bases['field_qbo_status'] = array(
    'active' => 1,
    'cardinality' => 1,
    'deleted' => 0,
    'entity_types' => array(),
    'field_name' => 'field_qbo_status',
    'indexes' => array(
      'value' => array(
        0 => 'value',
      ),
    ),
    'locked' => 0,
    'module' => 'list',
    'settings' => array(
      'allowed_values' => array(
        'not_synced' => 'Not synced',
        'pending' => 'Pending',
        'synced' => 'Synced',
      ),
      'allowed_values_function' => '',
    ),
    'translatable' => 0,
    'type' => 'list_text',
  );

  return $field_bases;
}

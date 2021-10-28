<?php

/**
 * Form for configuring the settings
 * 
 * @param type $form
 * @param type $form_state
 * @return type
 * 
 */

function commerce_quickbooks_settings_form($form, &$form_state) {

   $form['commerce_quickbooks_commerce_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Drupal Commerce cron'),
    '#default_value' => variable_get('commerce_quickbooks_commerce_cron'),
    '#description' => t('Warning - This setting will send all your customers, products and orders to QuickBooks online during cron runs.'),
   );
   
   // Build an array of order status options grouped by order state.
    $options = array();
    foreach (commerce_order_state_get_title() as $name => $title) {
      foreach (commerce_order_statuses(array('state' => $name)) as $order_status) {

        // Only include enabled statues and the current order status.
        if (!empty($order_status['status']) || $order->status == $order_status['name']) {
          $options[check_plain($title)][$order_status['name']] = check_plain($order_status['title']);
        }
      }
    }
    
    $form['commerce_quickbooks_order_status'] = array(
    '#type' => 'select',
    '#title' => t('Order status to send'),
     '#multiple' => TRUE,
    '#options' => $options,
    '#default_value' => variable_get('commerce_quickbooks_order_status'),
    '#description' => t('Select the order statuses that should be considered ready to send. You will want to select the final status. Not one that gets systematically updated. <br>This is because the queue gets built by querying the database for orders matching this status.'),
  );
   
   
   $form['commerce_quickbooks_comissions_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Comissions cron'),
    '#default_value' => variable_get('commerce_quickbooks_comissions_cron'),
    '#description' => t('Warning - This setting will send all your designers, affiliates and their comissions to QBO.'),
   );
   

  return(system_settings_form($form));

}

<?php 

  // Need for creating invoices using the QBO SDK
  use QuickBooksOnline\API\Core\ServiceContext;
  use QuickBooksOnline\API\DataService\DataService;
  use QuickBooksOnline\API\PlatformService\PlatformService;
  use QuickBooksOnline\API\Core\Http\Serialization\XmlObjectSerializer;
  use QuickBooksOnline\API\Facades\Invoice;
  
/**
 * This function is called during cron runs. See .module file.
 * 
 * @param type $data
 */

function commerce_quickbooks_sync_order ($data) {

  // Load required data
    $order_id = $data->order_id;
    $order = entity_load_single('commerce_order', $order_id);   
    $entity = entity_metadata_wrapper('commerce_order', $order);
    $invoice_number = $order->order_number;  
    $invoice_date = date('Y-m-d', $order->placed);
    $currency = variable_get("commerce_default_currency");
    $customer_uid = $order->uid;
    $customer = user_load($customer_uid);
    $customer_qbo_id = $customer->field_qbo_id['und'][0]['value'];
    $email = $order->mail;
    $courier = $order->field_commerce_order_courier['und'][0]['value'];
   
  // Get the orders billing profile information
    $billing = $entity->commerce_customer_billing->value();
    $billing_address = $billing->commerce_customer_address['und'][0];
    $billing_line_1 = empty($billing_address['thoroughfare']) ? "" : $billing_address['thoroughfare'];
    $billing_line_2 = empty($billing_address['premise']) ? "" : $billing_address['premise'];
    $billing_city = empty($billing_address['locality']) ? "" : $billing_address['locality'];
    $billing_county = empty($billing_address['county']) ? "" : $billing_address['county'];
    $billing_country = empty($billing_address['country']) ? "" : $billing_address['country'];
    $billing_postcode = empty($billing_address['postal_code']) ? "" : $billing_address['postal_code'];
    
  // Get the orders shipping profile information
    $shipping = $entity->commerce_customer_shipping->value();
    $shipping_address = $shipping->commerce_customer_address['und'][0];
    $shipping_line_1 = empty($shipping_address['thoroughfare']) ? "" : $shipping_address['thoroughfare'];
    $shipping_line_2 = empty($shipping_address['premise']) ? "" : $shipping_address['premise'];
    $shipping_city = empty($shipping_address['locality']) ? "" : $shipping_address['locality'];
    $shipping_county = empty($shipping_address['county']) ? "" : $shipping_address['county'];
    $shipping_country = empty($shipping_address['country']) ? "" : $shipping_address['country'];
    $shipping_postcode = empty($shipping_address['postal_code']) ? "" : $shipping_address['postal_code'];
     
  // Look up customer on QBO in order to get the current name.
    $qbo = new QBO();
    $qbo_customer = $qbo->getObject('Customer', $customer_qbo_id);
    $customer_qbo_name = $qbo_customer->DisplayName; 

  // Build Invoice array to send to QBO    
    $invoice = array();
    $invoice['CustomerRef'] = array('value' => $customer_qbo_id, 'name' => $customer_qbo_name);
    $invoice['DocNumber'] = $invoice_number;
    $invoice['TxnDate'] = $invoice_date;
    $invoice['CurrencyRef'] = array('value' => $currency);
    $invoice['BillEmail'] = array('Address' => $email);
    
     $invoice["BillAddr"] = [
       "Line1"=>  $billing_line_1,
       "Line2" => $billing_line_2,
       "City"=>  $billing_city,
       "Country"=>  $billing_country,
       "CountrySubDivisionCode"=>  $billing_county,
       "PostalCode"=>  $billing_postcode
     ];
      $invoice["ShipAddr"] = [
       "Line1"=>  $shipping_line_1,
       "Line2" => $shipping_line_2,
       "City"=>  $shipping_city,
       "Country"=>  $shipping_country,
       "CountrySubDivisionCode"=>  $shipping_county,
       "PostalCode"=>  $shipping_postcode
     ];

    $invoice['Line'] = array();
    
  // Loop over the orders line items
    foreach ($entity->commerce_line_items as $line_item_wrapper) {
      
        $invoice_line = array();
        $invoice_line['SalesItemLineDetail'] = array();
        
        $lineitem = $line_item_wrapper->value(); 
        $unit_amount = $lineitem->commerce_unit_price['und'][0]['amount'] / 100;
        $lineitem_total = $lineitem->commerce_total['und'][0]['amount'] / 100;
        $qty = $lineitem->quantity;
        
      // Get the line item product
        if ($lineitem->type == 'product') {
            $commerceproduct = $line_item_wrapper->commerce_product->value();
            $product_qbo_id = $commerceproduct->field_qbo_id['und'][0]['value'];

            $invoice_line['SalesItemLineDetail']['ItemRef'] = array('value' => $product_qbo_id);
            $invoice_line['Description'] = $commerceproduct->title;
        } 
        else if ($lineitem->type == 'shipping') {
            $invoice_line['SalesItemLineDetail']['ItemRef'] = array('value' => $courier);
            $invoice_line['Description'] = $lineitem->line_item_label;
        } 
        else if ($lineitem->type == 'commerce_discount') {
            $coupon_id = $order->commerce_coupons['und'][0]['target_id'];
            $coupon = entity_load_single('commerce_coupon', $coupon_id);
            $coupon_qbo_id = $coupon->field_qbo_id['und'][0]['value'];
            $invoice_line['SalesItemLineDetail']['ItemRef'] = array('value' => $coupon_qbo_id);
            $invoice_line['Description'] = 'Coupon code: ' . $coupon->code;
        }
        
        $invoice_line['DetailType'] = 'SalesItemLineDetail';
        $invoice_line['Amount'] = $lineitem_total;
        $invoice_line['SalesItemLineDetail']['Qty'] = $qty;
        $invoice_line['SalesItemLineDetail']['UnitPrice'] = $unit_amount;
        
        $invoice['Line'][] = $invoice_line;
    }
 
  // Send invoice to QBO  
    $qbo_invoice = Invoice::create($invoice);
    $resulting_invoice = $qbo->dataService()->Add($qbo_invoice);
    $error = $qbo->dataService()->getLastError();
    
  // Check for error and update the commerce order
    if ($error) {
      // Log error message in watchdog
        $message =  "There was a issue syncing order with ID: " . $order_id . ". <br>" .
                    "The Status code is: " . $error->getHttpStatusCode() . "<br>" . 
                    "The Helper message is: " . $error->getOAuthHelperError() . "<br>" . 
                    "The Response message is: " . $error->getResponseBody() . "<br>";
        watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_ERROR);
      // Check for duplicate error. Look up and store the existing QBO ID
      if (strpos($error->getResponseBody(), 'Duplicate Document Number') == TRUE) {
        $invoice = $qbo->query("SELECT Id FROM Invoice WHERE DocNumber = '" . $invoice_number . "'");
        if ($invoice) {
          $entity->field_qbo_id = $invoice[0]->Id;
          $entity->field_qbo_status = 'synced';
        } 
        else {
        $entity->field_qbo_status = 'not_synced';
        }   
      } 
      else {
      // Update product
        $entity->field_qbo_status = 'not_synced';
      }        
    }
    else {       
      // Log notice message in watchdog
        $message = "Synced! Commerce order with id " . $order_id;
        watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_NOTICE);
       // Update order QBO status
          $entity->field_qbo_status = 'synced';
          $entity->field_qbo_id = $resulting_invoice->Id;
    }
    // Save the commerce order
      $entity->save();  
}

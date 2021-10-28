<?php 

  // Required to use QBO SDK - CustomerCreate
    use QuickBooksOnline\API\Core\ServiceContext;
    use QuickBooksOnline\API\DataService\DataService;
    use QuickBooksOnline\API\PlatformService\PlatformService;
    use QuickBooksOnline\API\Core\Http\Serialization\XmlObjectSerializer;
    use QuickBooksOnline\API\Facades\Customer;
    
/**
 * This function is called during cron runs. See .module file.
 * 
 * @param type $data
 */

function commerce_quickbooks_sync_customer ($data) {
   
    $store_currency = variable_get("commerce_default_currency");
    
  // Load customer account
    $uid = $data->uid;
    $user = user_load($uid);
    $user_email = $user->mail;
    $user_update = array();
  
  // Create a QBO connection
    $qbo = new QBO();
    
  // Check if we have customer in QBO from another channel.
    $lookup_query = "SELECT * FROM Customer WHERE PrimaryEmailAddr  = '" . $user_email . "'";
    $customer_lookup = $qbo->dataService()->Query($lookup_query);
    $qbo_id = '';

  
    if ($customer_lookup) {
        foreach ($customer_lookup as $customer) {
            $currency = $customer->CurrencyRef;
            if ($currency = $store_currency) {
                $qbo_id = $customer->Id;
                $user_update['field_qbo_id'][LANGUAGE_NONE][0]['value'] = $qbo_id;
                $user_update['field_qbo_status'][LANGUAGE_NONE][0]['value'] = 'synced';
            };
        } 
    } 
    
    if (empty($qbo_id)){
        
    // Retrieve the oldest order that was invoiced.
      $valid_statuses = commerce_quickbooks_get_statuses();
      $order_id = db_query("SELECT order_id 
                        FROM commerce_order
                        WHERE uid = :uid
                        AND status in (:statuses)
                        ORDER BY placed ASC
                        LIMIT 1", array(':uid' => $uid, ':statuses' => $valid_statuses))->fetchField();
      $order = commerce_order_load($order_id);
      $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

    // Get the orders billing profile information
      $billing = $order_wrapper->commerce_customer_billing->value();
      $billing_address = $billing->commerce_customer_address['und'][0];
      $billing_first_name = empty($billing_address['first_name']) ? "" : $billing_address['first_name'];
      $billing_last_name = empty($billing_address['last_name']) ? "" : $billing_address['last_name'];
      $billing_full_name = empty($billing_address['name_line']) ? "" : $billing_address['name_line'];
      $billing_company = empty($billing_address['organisation_name']) ? "" : $billing_address['organisation_name'];
      $billing_line_1 = empty($billing_address['thoroughfare']) ? "" : $billing_address['thoroughfare'];
      $billing_line_2 = empty($billing_address['premise']) ? "" : $billing_address['premise'];
      $billing_city = empty($billing_address['locality']) ? "" : $billing_address['locality'];
      $billing_county = empty($billing_address['county']) ? "" : $billing_address['county'];
      $billing_country = empty($billing_address['country']) ? "" : $billing_address['country'];
      $billing_postcode = empty($billing_address['postal_code']) ? "" : $billing_address['postal_code'];

    // Get the orders shipping profile information
      $shipping = $order_wrapper->commerce_customer_shipping->value();
      $shipping_address = $shipping->commerce_customer_address['und'][0];
      $shipping_line_1 = empty($shipping_address['thoroughfare']) ? "" : $shipping_address['thoroughfare'];
      $shipping_line_2 = empty($shipping_address['premise']) ? "" : $shipping_address['premise'];
      $shipping_city = empty($shipping_address['locality']) ? "" : $shipping_address['locality'];
      $shipping_county = empty($shipping_address['county']) ? "" : $shipping_address['county'];
      $shipping_country = empty($shipping_address['country']) ? "" : $shipping_address['country'];
      $shipping_postcode = empty($shipping_address['postal_code']) ? "" : $shipping_address['postal_code'];
      $shipping_phone = empty($shipping->field_commerce_telephone) ? "" : $shipping->field_commerce_telephone['und'][0]['value']; 

    // Specify Display name for this customer in QBO 
      $display_name = 'C - ' . $billing_full_name . ' - ' . $user->name . ' - ' . $store_currency;

    // Build customer array to send to QBO
      $qbo_customer = Customer::create([
      "BillAddr" => [
         "Line1"=>  $billing_line_1,
         "Line2" => $billing_line_2,
         "City"=>  $billing_city,
         "Country"=>  $billing_country,
         "CountrySubDivisionCode"=>  $billing_county,
         "PostalCode"=>  $billing_postcode
       ],
        "ShipAddr" => [
         "Line1"=>  $shipping_line_1,
         "Line2" => $shipping_line_2,
         "City"=>  $shipping_city,
         "Country"=>  $shipping_country,
         "CountrySubDivisionCode"=>  $shipping_county,
         "PostalCode"=>  $shipping_postcode
       ],
       "GivenName"=>  $billing_first_name,
       "FamilyName"=>  $billing_last_name,
       "FullyQualifiedName"=>  $billing_full_name,
       "CompanyName"=>  $billing_company,
       "DisplayName"=> $display_name,
       "PrimaryPhone"=>  [
           "FreeFormNumber"=>  $shipping_phone
       ],
       "PrimaryEmailAddr"=>  [
           "Address"=> $user_email
       ],
       "CurrencyRef"=> $store_currency
      ]);

      // Send new customer to QBO
        $new_qbo_customer = $qbo->dataService()->Add($qbo_customer);
        $error = $qbo->dataService()->getLastError();
    
      // Update customer QBO status and/or QBO ID
        if ($error) {
          // Log error message in watchdog.
            $message = "There was a issue syncing customer with uid: " . $uid . ". <br>"
                      . "The Status code is: " . $error->getHttpStatusCode() . "<br> "
                      . "The Helper message is: " . $error->getOAuthHelperError() . "<br>"
                      . "The Response message is: " . $error->getResponseBody() . "<br>";
            watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_ERROR);
            
          // Set user to not synced
            $user_update['field_qbo_status'][LANGUAGE_NONE][0]['value'] = 'not_synced';
        } else {
          // Set user to synced and save the QBO Id
            $qbo_id = $new_qbo_customer->Id;
            $user_update['field_qbo_id'][LANGUAGE_NONE][0]['value'] = $qbo_id;
            $user_update['field_qbo_status'][LANGUAGE_NONE][0]['value'] = 'synced';
        }
    }
    // Save Drupal user
     user_save($user, $user_update); 
}

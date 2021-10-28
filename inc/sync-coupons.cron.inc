<?php 

// Needed to make use of the QBO SDK to create a item/product
  use QuickBooksOnline\API\Core\ServiceContext;
  use QuickBooksOnline\API\DataService\DataService;
  use QuickBooksOnline\API\PlatformService\PlatformService;
  use QuickBooksOnline\API\Core\Http\Serialization\XmlObjectSerializer;
  use QuickBooksOnline\API\Facades\Item;

/**
 * This function is called during cron runs. See .module file.
 * 
 * @param type $data
 */

function commerce_quickbooks_sync_coupon($coupon) {
  // Initialise the QBO conntection
    $qbo = new QBO();
    
  // Load the commerce product
    $coupon_id = $coupon->coupon_id;
    $entity = entity_metadata_wrapper('commerce_coupon', $coupon);
    $title = $coupon->code;
    $sku = 'coupon_' . $coupon_id;
    $unit_price = 0;
    $income_account_id = $coupon->field_product_qbo_income_account['und'][0]['value'];
    $qbo_account = $qbo->getObject('Account', $income_account_id);
    $qbo_account_name = $qbo_account->Name;
    
    
    
  // Build the item object to send to QBO
    $Item = Item::create([
        "Name" => $title,
        "Sku" => $sku,
        "Active" => true,
        "UnitPrice" => $unit_price,
        "Type" => "NonInventory",
        "IncomeAccountRef"=> [
          "value"=> $income_account_id,
          "name" => $qbo_account_name
        ],
        "TrackQtyOnHand" => false
  ]);

  // Send new product to QBO
    
    $resultingObj = $qbo->dataService()->Add($Item);
    $error = $qbo->dataService()->getLastError();

  if ($error) {
    // Log error message in watchdog.
      $message = "There was a issue syncing product with ID: " . $coupon_id . ". <br>"
                . "The Status code is: " . $error->getHttpStatusCode() . "<br> "
                . "The Helper message is: " . $error->getOAuthHelperError() . "<br>"
                . "The Response message is: " . $error->getResponseBody() . "<br>";
      watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_ERROR);
    // Update product
      $entity->field_qbo_status = 'not_synced';
  }
  else {
    // Log notice message in watchdog
      $message = "Synced! Product with id " . $coupon_id;
     // watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_NOTICE);
    // Update product
      $qbo_id = $resultingObj->Id;
      $entity->field_qbo_status = 'synced';
      $entity->field_qbo_id = $qbo_id;
  }
  
  //Save product
  $entity->save();
  
}

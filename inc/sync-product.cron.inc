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

function commerce_quickbooks_sync_product ($product) {
  
  // Initialise the QBO conntection
    $qbo = new QBO();
    
  // Load the commerce product
    $store_currency = variable_get("commerce_default_currency");
    $entity = entity_metadata_wrapper('commerce_product', $product);
    $title = $product->title;
    $sku = $product->sku . " - " . $store_currency;
    $qbo_id = '';
    
   // Check if we have a item / product in QBO from another sales channel.
    $product_query = "SELECT * FROM Item WHERE Sku = '" . $sku . "'";
    $product_lookup = $qbo->dataService()->Query($product_query);
    
   // If found, update commerce product with QBO Id
    if ($product_lookup) {
        foreach ($product_lookup as $qbo_item) {
            $qbo_id = $qbo_item->Id;
            $entity->field_qbo_status = 'synced';
            $entity->field_qbo_id = $qbo_id;
        } 
    } 
    // If product not found in QBO, create it.
    if (empty($qbo_id)){
    // Set local variables for Commerce product    
        $unit_price = $product->commerce_price['und'][0]['amount'] / 100;
        $income_account_id = $product->field_product_qbo_income_account['und'][0]['value'];
        $qbo_account = $qbo->getObject('Account', $income_account_id);
        $qbo_account_name = $qbo_account->Name;
        $product_id = $product->product_id;
    
    // Build the item object to send to QBO
        $item = Item::create([
            "Name" => $sku,
            "Sku" => $sku,
            "Description" => $title,
            "Active" => true,
            "UnitPrice" => $unit_price,
            "Type" => "NonInventory",
            "IncomeAccountRef"=> [
              "value"=> $income_account_id,
              "name" => $qbo_account_name
            ],
            "TrackQtyOnHand" => false
        ]);

    // Send new item to QBO
      $new_qbo_item = $qbo->dataService()->Add($item);
      $error = $qbo->dataService()->getLastError();

    if ($error) {
      // Log error message in watchdog.
        $message = "There was a issue syncing product with ID: " . $product_id . ". <br>"
                  . "The Status code is: " . $error->getHttpStatusCode() . "<br> "
                  . "The Helper message is: " . $error->getOAuthHelperError() . "<br>"
                  . "The Response message is: " . $error->getResponseBody() . "<br>";
        watchdog('commerce_quickbooks', $message, $variables = array(), WATCHDOG_ERROR);
      // Reset QBO status on commerce product
        $entity->field_qbo_status = 'not_synced';
    }
    else {
      // Update commerce product
        $qbo_id = $new_qbo_item->Id;
        $entity->field_qbo_status = 'synced';
        $entity->field_qbo_id = $qbo_id;
    }
 }
  
  //Save commerce product
    $entity->save();
  
}

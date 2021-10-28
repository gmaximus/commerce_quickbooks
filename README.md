# Commerce Quickbooks for Drupal 7
## Intro
This Drupal 7 module doesn't support multiple currencies. Nor does it support anonymous checkouts. 

Essentially this module will send orders, products and users (Customers) over to Quickbooks. It will do this on Cron runs, so as to not delay customers.

The module provides a simple configuration form. You merely tick a checkbox to enable the cron and select what order statuses should be sent over.

Throughout these instructions, replace example.com with your actual domain name. I'll focus on setting up a sandbox but the live setup is identical.

## Setup Quickbooks Online API module (if not already achieved).
1. Install Quckbooks API requirements. These are [oauth2_client](https://www.drupal.org/project/oauth2_client) and [reqistry_autoload](https://www.drupal.org/project/registry_autoload). 
2. Download the [QuickBooks-V3-PHP-SDK
](https://github.com/intuit/QuickBooks-V3-PHP-SDK/tree/v4.0.6.1). Rename the folder to qbo and place it in the libraries folder. You should have a structure like this:
- sites/all/libraries
- sites/all/libraries/qbo
- sites/all/libraries/qbo/src
3. Download Quickbooks API from [here](https://www.drupal.org/project/qbo_api/issues/2936266#comment-12554554)
4. Visit [Intuit Developer](https://developer.intuit.com/app/developer/dashboard) to create your app. Sign in using your main Quickbooks log in details.
5. Under "Developer" on the left side menu, click "Keys & OAuth".
6. Configure the URL's as follows:

- Redirect URI https://example.com/qbo/oauth/prod
- Host domain: example.com
- Launch URL: https://example.com/qbo/oauth
- Disconnect URL: https://example.com/qbo/oauth

7. Visit admin/config/services/qbo and enter the two keys.
8. Last thing we need is a company ID. Back on Intuit, mouse over "API Docs and Tools" in the top right and click "Sandbox".
9. Now click "Add a sandbox company". Be sure you select the correct country and currency. Then click it to sign into the new sandbox company.
10. Mouse over the top right clog, then click "Accounts and settings". It's under the Your Company section.
11. Right side now, click Billing and Subscriptions. Copy the Company number from the top of the page into the Drupal settings page and click save.
12. Finally to make the connection, visit https://example.com/qbo/oauth/prod and follow the on-screen instructions. The end of the process is a blank white screen, it's expected for some reason. Revisit the configuration page to confirm successful connection.

## Set up Commerce Quickbooks
### Enable these modules
1. Enable Commerce Quickbooks
2. Enable Commerce Quickbooks Feature.

### Set up Commerce Shipping (if relevant)
1. Create products in QBO for the shipping companies you use ie FedEx, UPS, Royal Mail. Ensure the products are the same currency as your commerce store.
2. Note the QBO product IDs. You can find that in the URL when editing the products in Quickbooks. You can also execute this PHP. Just switch out the example_sku.
```
$qbo = new QBO();
$sku = "example_sku";
$product_query = "SELECT * FROM Item WHERE Sku = '" . $sku . "'";
$product_lookup = $qbo->dataService()->Query($product_query);
dpm($product_lookup);
```
3. Create a select list field on orders with the machine name "field_commerce_order_courier".
4. For keys you **must** use the product IDs from step 2. For the human friendly values, it can be anything. 
Personally I have the HTML for a link to the tracking page for the relevant courier. Then show the field to customers. Along with a separate field for the tracking numbers.

### Set up user and Commerce entities
5. Add the field_product_qbo_income_account select list field to every additional product type.
6. Execute this PHP code. It will then always call Quickbooks for a list of accounts. You may want to use the [devel](https://drupal.org/project/devel) module to execute the PHP.
```
$field = field_info_field('field_product_qbo_income_account');
$field['settings']['allowed_values_function'] = 'commerce_quickbooks_fetch_accounts';
field_update_field($field);
```
7. Update the income account on all products. 
8. Update the QBO status on all users, products and orders to "not synced".

### Activate cron job
9. Turn on the cron job and select what order statuses to send at admin/config/services/qbo/commerce_quickbooks

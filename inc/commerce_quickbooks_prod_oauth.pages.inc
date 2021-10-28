<?php

use \QuickBooksOnline\API\Core\ServiceContext;

/**
 * Page callback which calls OAuth library to request and set the access
 * key and secret tokens.
 */
function commerce_quickbooks_prod_oauth_page() {
  global $base_root;

  // Load inc file. We need this for our custom getToken()
  module_load_include('inc', 'qbo_api', 'includes/qboOauthClient');
 $context = "QBOContextGlobal";
  // Determine the context.
  
    try {
      // Initialize QBO.
      $qbo = new QBO();

      // Check access.
      if (!$qbo->context->access()) {
        // Deny access.
        drupal_access_denied();
      }
    }
    catch (QBOException $e) {
      drupal_access_denied();
    }


  // Extract the QBO keys.
  extract($qbo->keys);

  // We can't generate a token if the consumer keys aren't set.
  if (empty($consumer_key) || empty($consumer_secret)) {
    print t('OAuth consumer key and secret token must be set before requesting access tokens.');
    drupal_exit();
  }

  // Create config for oauth2_client
  // server-side flow
  $oauth2_clients['qbo'] = array(
    'token_endpoint' => QBO_API_OAUTH_URL_REQUEST,
    'auth_flow' => 'server-side',
    'client_id' => $consumer_key,
    'client_secret' => $consumer_secret,
    'scope' => 'com.intuit.quickbooks.accounting',
    'response_type' => 'code',
    'authorization_endpoint' => QBO_API_OAUTH_URL_AUTHORISE,
    'redirect_uri' => $base_root . request_uri(),
  );


  try {
    // Build the OAuth connection.
    if (!module_exists('oauth2_client')) {
      $oauth = new OAuth($consumer_key, $consumer_secret, OAUTH_SIG_METHOD_HMACSHA1, OAUTH_AUTH_TYPE_URI);
      $oauth->enableDebug();

      // To avoid the error: (Peer certificate cannot be authenticated with given CA certificates).
      $oauth->disableSSLChecks();
    } else {
      // We are using oauth2_client module
      // $oauth = new OAuth2\Client($oauth2_clients['qbo'], $consumer_key);
      $oauth = new OAuth2\qboOauthClient($oauth2_clients['qbo'], $consumer_key);
    }

    if (module_exists('oauth2_client')) {
      $token = $oauth->getAccessToken();
      $refresh_token = $_SESSION['oauth2_client']['token'][$consumer_key]['refresh_token'];

      // Save the OAuth keys.
      $qbo->context->save(array(
        'access_token' => $refresh_token, // This is different from original
        'access_secret' => $token, // This is the real access token
        'auth_date' => REQUEST_TIME,
        'refresh_expire' => time() + $_SESSION['oauth2_client']['token'][$consumer_key]['x_refresh_token_expires_in'],
      ));

      // Delete the token in the session.
      unset($_SESSION['qbo_api_secret']);

      // Use JS to refresh parent and close popup.
      echo '<script type="text/javascript">
              window.opener.location.href = window.opener.location.href;
              window.close();
            </script>';

      // The below is no longer needed.
    } else {
      if (!isset($_GET['oauth_token']) && empty($access_token)) {
        // Get request token from QBO.
        // $request_token = $oauth->getRequestToken(QBO_API_OAUTH_URL_REQUEST, $base_root . request_uri());
        // Change Andreas
        $request_token = $oauth->getRequestToken(QBO_API_OAUTH_URL_AUTHORISE, $base_root . request_uri(), 'GET');

        // Store the token for later.
        $_SESSION['qbo_api_secret'] = $request_token['oauth_token_secret'];
 
        // Send user to QBO to authorize.
        header('Location: ' . QBO_API_OAUTH_URL_AUTHORISE . '?oauth_token=' . $request_token['oauth_token']);
      }
    }

    if (isset($_GET['oauth_token']) && isset($_GET['oauth_verifier'])) {
      // Request an access token from QBO.
      $oauth->setToken($_GET['oauth_token'], $_SESSION['qbo_api_secret']);
      $access_token = $oauth->getAccessToken(QBO_API_OAUTH_URL_ACCESS);

      // Save the OAuth keys.
      $qbo->context->save(array(
        'access_token' => $access_token['oauth_token'],
        'access_secret' => $access_token['oauth_token_secret'],
        'auth_date' => REQUEST_TIME,
        'company_id' => $_REQUEST['realmId'],
      ));

      // Delete the token in the session.
      unset($_SESSION['qbo_api_secret']);

      // Use JS to refresh parent and close popup.
      echo '<script type="text/javascript">
              window.opener.location.href = window.opener.location.href;
              window.close();
            </script>';
    }
  }
  catch (OAuthException $e) {
    watchdog('qbo_api', $e->getMessage(), array(), WATCHDOG_ERROR);
    print t('An error occurred.');
    drupal_exit();
  }
}

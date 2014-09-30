<?php
/**
 * API and hooks documentation for the Commerce Litle module.
 */

class VantivApi {
  /**
   * Set merchant credentials.
   */
  public function __construct($settings) {
    $this->account = !empty($settings['account']) ? trim($settings['account']) : '';
    $this->user = !empty($settings['user']) ? trim($settings['user']) : '';
    $this->password = !empty($settings['password']) ? trim($settings['password']) : '';
    $this->merchant_id = !empty($settings['merchant_id']) ? trim($settings['merchant_id']) : '';
    $this->three_d = !empty($settings['3d_secure']) ? trim($settings['3d_secure']) : '';
    $this->transaction_type_process = !empty($settings['transaction_type_process']) ? trim($settings['transaction_type_process']) : '';
    $this->logs = !empty($settings['logs']) ? trim($settings['logs']) : '';
  }

  /**
   * Loads the library functions needed to process transactions.
   */
  public function libraryLoad() {
    define('STDIN',fopen("php://stdin","r"));
    $library = libraries_info('litle');
    $load_library = libraries_load('litle');
    libraries_load_files($load_library);
  }

  /**
   * Process direct payments.
   */
  public function directPayments($customer_profile, $order, $card_info, $payment_method, $amount = '') {
    $load_library = self::libraryLoad();
    $currency_code = empty($amount->currency_code) ? $order->commerce_order_total['und'][0]['currency_code'] : $amount->currency_code;
    $charge_amount = empty($amount->amount) ? $order->commerce_order_total['und'][0]['amount'] : $amount->amount;
    $card_owner_name = $customer_profile->commerce_customer_address['und'][0]['name_line'];
    $user = user_load($order->uid);
    if (empty($token)) {
      $billing_data = array(
        'user' => $this->user,
        'password' => $this->password,
        'merchantId' => $this->merchant_id,
        'reportGroup' => 'rtpGrp',
        'url' => $this->accountLinks($this->account),
        'litle_requests_path' => $this->accountLinks($this->account),
        'batch_requests_path' => '',
        'proxy' => '',
        'sftp_username' => '',
        'sftp_password' => '',
        'batch_url' => '',
        'tcp_port' => '',
        'tcp_ssl' => '',
        'tcp_timeout' => '',
        'print_xml' => '',
        'orderId' => $order->order_id,
  		  'amount' => $charge_amount,
        //id of auth or sale response in the xml tag.
        'id'=> $order->order_id . '|' . time(),
  		  'orderSource'=>'ecommerce',
  		  'billToAddress'=>array(
  		  'name' => $customer_profile->commerce_customer_address['und'][0]['name_line'],
  		  'addressLine1' => $customer_profile->commerce_customer_address['und'][0]['thoroughfare'],
  		  'city' => $customer_profile->commerce_customer_address['und'][0]['locality'],
  		  'state' => $customer_profile->commerce_customer_address['und'][0]['administrative_area'],
  		  'zip' => $customer_profile->commerce_customer_address['und'][0]['postal_code'],
  		  'country' => $customer_profile->commerce_customer_address['und'][0]['country']),
  		  'card' => array(
  		  'number' => $card_info['credit_card']['number'],
  		  'expDate' => $card_info['credit_card']['exp_month'] . substr($card_info['credit_card']['exp_year'], 2, 4),
  		  'cardValidationNum' => $card_info['credit_card']['code'],
  		  'type' => $this->cardTypes($card_info['credit_card']['type'])),
      );
    }
    else {
      $billing_data = array(
        'user' => $this->user,
        'password' => $this->password,
        'merchantId' => $this->merchant_id,
        'reportGroup' => 'rtpGrp',
        'url' => $this->accountLinks($this->account),
        'litle_requests_path' => $this->accountLinks($this->account),
        'batch_requests_path' => '',
        'proxy' => '',
        'sftp_username' => '',
        'sftp_password' => '',
        'batch_url' => '',
        'tcp_port' => '',
        'tcp_ssl' => '',
        'tcp_timeout' => '',
        'print_xml' => '',
        'orderId' => $order->order_id,
  		  'amount' => $charge_amount,
        //id of auth or sale response in the xml tag.
        'id'=> $order->order_id . '|' . time(),
  		  'orderSource'=>'ecommerce',
  		  'billToAddress'=>array(
  		  'name' => $customer_profile->commerce_customer_address['und'][0]['name_line'],
  	    'addressLine1' => $customer_profile->commerce_customer_address['und'][0]['thoroughfare'],
  		  'city' => $customer_profile->commerce_customer_address['und'][0]['locality'],
  		  'state' => $customer_profile->commerce_customer_address['und'][0]['administrative_area'],
  		  'zip' => $customer_profile->commerce_customer_address['und'][0]['postal_code'],
  		  'country' => $customer_profile->commerce_customer_address['und'][0]['country']),
        'token' => array(
          'litleToken' => $token,
        ),
      );
    }
    $initilaize = new litle\sdk\LitleOnlineRequest();
    if ($echeck != TRUE) {
      if ($this->transaction_type_process == 'pre_auth' /*and empty($card_info['credit_card']['cardonfile_store'])*/ and (is_object($card_info) == TRUE or empty($card_info['credit_card']['cardonfile_store']))) {
        $response = $initilaize->authorizationRequest($billing_data);
      }
      elseif ($this->transaction_type_process == 'sale' /*and empty($card_info['credit_card']['cardonfile_store'])*/and (is_object($card_info) == TRUE or empty($card_info['credit_card']['cardonfile_store']))) {
        $response = $initilaize->saleRequest($billing_data);
      }
      elseif (!empty($card_info['credit_card']['cardonfile_store']) /*and $card_info['credit_card']['cardonfile_store'] == TRUE*/ and is_object($card_info) != TRUE and is_array($card_info) == TRUE and $card_info['credit_card']['cardonfile_store'] == TRUE) {
        $billing_data['accountNumber'] = $card_info['credit_card']['number'];
        $response = $initilaize->registerTokenRequest($billing_data);
      }
      if (!empty($response)) {
        $this->processDirectTransactions($response, $payment_method, '', $card_info);
      }
      else {
        watchdog('commerce_vantiv', "An error occur with order# @order during the request, the server did not respond.", array('@order' => $order->order_id), WATCHDOG_ERROR);
      }
    }
    elseif ($echeck == TRUE) {
      if  (module_exists('commerce_shipping')) {
        unset($customer_profile);
        $customer_profile = commerce_customer_profile_load($order->commerce_customer_shipping['und'][0]['profile_id']);
      }
      $billing_data['shipToAddress'] = array(
        'name' => $customer_profile->commerce_customer_address['und'][0]['name_line'],
        'city' => $customer_profile->commerce_customer_address['und'][0]['locality'],
        'state' => '',
        'email' => $order->mail,
      );

      $billing_data['echeck'] = array(
        'accType' => $card_info['accType'],
        'accNum' => $card_info['accNum'],
        'routingNum' => $card_info['routingNum'],
        'checkNum' => $card_info['checkNum'],
      );

      $response = $initilaize->echeckSaleRequest($billing_data);
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
      if (!empty($response)) {
        $this->processDirectTransactions($response, $payment_method, '', $card_info);
      }
      else {
        watchdog('commerce_vantiv', "An error occur with order# @order during the request, the server did not respond.", array('@order' => $order->order_id), WATCHDOG_ERROR);
      }
    }
  }

  /**
   * Process cross payments.
   */
  public function crossPayment($transaction, $partial = '', $amount = '', $type = '') {
    $load_library = self::libraryLoad();

    $billing_data = array(
          'user' => $this->user,
          'password' => $this->password,
          'merchantId' => $this->merchant_id,
          'reportGroup' => 'rtpGrp',
          'url' => $this->accountLinks($this->account),
          'litle_requests_path' => $this->accountLinks($this->account),
          'batch_requests_path' => '',
          'proxy' => '',
          'sftp_username' => '',
          'sftp_password' => '',
          'batch_url' => '',
          'tcp_port' => '',
          'tcp_ssl' => '',
          'tcp_timeout' => '',
          'print_xml' => '',
          'id'=> $transaction->data['id'],
          'litleTxnId' => $transaction->remote_id,
    );

    // Add additional parameters for partial capture and refund transactions.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'capture') {
      $billing_data['partial'] = 'true';
      $billing_data['amount'] = $amount;
      $partial_amount = $amount;
    }
    elseif (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'refund') {
      $billing_data['amount'] = $amount;
      $partial_amount = $amount;
    }
    elseif (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'reversal') {
      $billing_data['amount'] = $amount;
      $partial_amount = $amount;
    }

    $init = new litle\sdk\LitleOnlineRequest();

    // Perform the http request.
    switch ($type) {
      case 'capture':
        $response = $init->captureRequest($billing_data);
        break;
      case 'refund':
        $response = $init->creditRequest($billing_data);
        break;
      case 'void':
        $response = $init->voidRequest($billing_data);
        break;
      case 'reversal':
        $response = $init->authReversalRequest($billing_data);
        break;
    }

    $payment_method = commerce_payment_method_instance_load($transaction->instance_id);

    // Saves partial or fully captured transaction.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'capture') {
      $this->processCapturedTransactions($response, $payment_method, $transaction, $partial_amount);
    }
    elseif ($type == 'capture') {
      $this->processCapturedTransactions($response, $payment_method, $transaction);
    }

    // Saves partial or fully refund transaction.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'refund') {
      $this->processRefundedTransactions($response, $payment_method, $transaction, $partial_amount);
    }
    elseif ($type == 'refund') {
      $this->processRefundedTransactions($response, $payment_method, $transaction);
    }

    // Updates voided transaction.
    if ($type == 'void') {
      $this->processVoidTransaction($response, $payment_method, $transaction);
    }

    // Saves partial or full reversed auth transaction.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'reversal') {
      $this->processReversedTransactions($response, $payment_method, $transaction, $partial_amount);
    }
    elseif ($type == 'reversal') {
      $this->processReversedTransactions($response, $payment_method, $transaction);

    }
  }

//vantiv_direct|commerce_payment_vantiv_direct
  /**
   * Process the transaction according to the respond.
   */
  public function processDirectTransactions($response, $payment_method, $transaction = '') {
      // Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $order_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $authorizationResponse = litle\sdk\XmlParser::getNode($response,'authorizationResponse');
    $saleResponse = litle\sdk\XmlParser::getNode($response,'saleResponse');
    $registerTokenResponse = litle\sdk\XmlParser::getNode($response,'registerTokenResponse');
    $echeckSalesResponse = litle\sdk\XmlParser::getNode($response,'echeckSalesResponse');

    // Get new data from account updater if changes were made.
    $accountUpdater = litle\sdk\XmlParser::getNode($response, 'accountUpdater');
    $newCardInfo = litle\sdk\XmlParser::getNode($response, 'newCardInfo');
    $type = litle\sdk\XmlParser::getNode($response, 'type');
    $number = litle\sdk\XmlParser::getNode($response, 'number');
    $expDate = litle\sdk\XmlParser::getNode($response, 'expDate');

    if (!empty($authorizationResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'authorizationResponse', 'id');
      $payload_attribute = 'authorizationResponse';
    }
    if (!empty($captureResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'captureResponse', 'id');
      $payload_attribute = 'captureResponse';
    }
    if (!empty($registerTokenResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'registerTokenResponse', 'id');
      $payload_attribute = 'registerTokenResponse';
      $litleToken = litle\sdk\XmlParser::getNode($response,'litleToken');
    }
    if (!empty($echeckSalesResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'echeckSalesResponse', 'id');
      $payload_attribute = 'echeckSalesResponse';
    }
    elseif (!empty($echeckSalesResponse)) {
      $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $transaction->message .= t('Sale:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
    }
    //If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id)) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }
      // If the transaction is not registerTokenResponse.
      if (empty($registerTokenResponse)) {
        $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
        $transaction->instance_id = $payment_method['instance_id'];
        $transaction->amount = $order->commerce_order_total['und'][0]['amount'];
        $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
        $transaction->remote_id = $litle_txn_id;
        $transaction->remote_status = $message;
        $transaction->message_variables = array('@auth_code' => $auth_code);
        $transaction->data = array('auth_code' => $auth_code);

        //Get the payload.
        $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
        $payload_array = $this->convertPayloadToArray($payload, $payload_attribute);
        $transaction->payload = (is_array($payload_array) == TRUE) ? $payload_array : $payload;

        if (!empty($authorizationResponse)) {
          $transaction->status = COMMERCE_PAYMENT_STATUS_PENDING;
          $transaction->message .= t('Authorized:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
          $transaction->data = array('id' => $id);
        }
        elseif (!empty($saleResponse)) {
          $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
          $transaction->message .= t('Sale:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        }
        elseif (!empty($saleResponse)) {
          $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
          $transaction->message .= t('Sale:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        }
        else {
          if (!empty($message) and !empty($order)) {
            if ($this->logs == 0) {
              watchdog('commerce_vantiv', "The order# @order was cancelled, due to @message", array('@order' => $order->order_id, '@message' => $message), WATCHDOG_ERROR);
            }
            $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
          }
          drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
        }
        commerce_payment_transaction_save($transaction);
      }
      // Save card on file if required.
      if (!empty($litleToken)) {
        $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
        $card_data = commerce_cardonfile_new();
        $card_data->uid = !empty($order->uid) ? $order->uid : $user->uid;
        $card_data->remote_id = $litleToken;
        $card_data->payment_method = $payment_method['method_id'];
        $card_data->instance_id = $payment_method['instance_id'];
        $card_data->card_type = !empty($card_info['credit_card']['type']) ? strtolower($card_info['credit_card']['type']) : '';
        $card_data->card_name = !empty($customer_profile->commerce_customer_address[LANGUAGE_NONE][0]['name_line']) ? $customer_profile->commerce_customer_address[LANGUAGE_NONE][0]['name_line'] : $order['credit_card']['owner']['#value'];
        $card_data->card_number = drupal_substr($card_info['credit_card']['number'], -4);
        $card_data->card_exp_month = $card_info['credit_card']['exp_month'];
        $card_data->card_exp_year = $card_info['credit_card']['exp_year'];
        $card_data->status = 1;
        if (!empty($accountUpdater) and !empty($newCardInfo)) {
          $card_type = $api->cardTypes('', $type);
          $exp_month = substr($expDate, 1, 2);
          $exp_year = substr($expDate, -2, 2);
          $card_data->card_name = $params['credit_card']['owner'];
          $card_data->card_number = substr($number, -4, 4);
          $card_data->type = !empty($card_type) ? $card_type : '';
          $card_data->card_exp_month = $exp_month;
          $card_data->card_exp_year = $exp_year;
        }
        commerce_cardonfile_save($card_data);
        $card_info['cardonfile_store'] = '';
        $this->directPayments($customer_profile, $order, '', $payment_method, '', $litleToken);
      }
    }
  }

  /**
   * Process captured transactions.
   */
  public function processCapturedTransactions($response, $payment_method, $transaction, $partial_amount = '') {
    //Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $order_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $captureResponse = litle\sdk\XmlParser::getNode($response,'captureResponse');

    if (!empty($captureResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'captureResponse', 'id');
      $payload_attribute = 'captureResponse';
    }

    // If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id)) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }
      // The pending transaction becomes captured.
      $transaction->status = COMMERCE_VANTIV_STATUS_CAPTURED;
      $previous_transaction_amount = $transaction->amount;
      // Set the new transaction balance of the previous transaction from the same order.
      if (!empty($partial_amount)) {
        $transaction->amount = $transaction->amount - $partial_amount;
      }
      else {
        $transaction->amount = 0;
      }

      commerce_payment_transaction_save($transaction);

      $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
      if (!empty($partial_amount)) {
        $transaction->amount = $partial_amount;
      }
      else {
        $transaction->amount = $previous_transaction_amount;
      }

      $transaction->instance_id = $payment_method['instance_id'];
      $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
      //$transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $transaction->remote_id = $litle_txn_id;
      $transaction->remote_status = $message;
      $transaction->message_variables = array('@auth_code' => $auth_code);

      // Get the payload.
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
      $payload_array = $this->convertPayloadToArray($payload, $payload_attribute);
      $transaction->payload = (is_array($payload_array) == TRUE) ? $payload_array : $payload;

      if (!empty($captureResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_CAPTURE;
        $transaction->message .= t('<br/> Captureed:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
        drupal_set_message(t('Transaction was captured successfully.'), 'status');
      }
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_vantiv', "This operation was not successful due to: @message", array('@message' => $message), WATCHDOG_ERROR);
          }
        }
        drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
      }
      commerce_payment_transaction_save($transaction);
      drupal_goto('admin/commerce/orders');
    }
  }

  /**
   * Process refunded transactions.
   */
  public function processRefundedTransactions($response, $payment_method, $transaction, $partial_amount = '') {
    //Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $order_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $creditResponse = litle\sdk\XmlParser::getNode($response,'creditResponse');

    if (!empty($creditResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'creditResponse', 'id');
      $payload_attribute = 'creditResponse';
    }

    //If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id)) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }
      $previous_transaction_amount = $transaction->amount;
      // The captured transaction becomes refunded.
      $transaction->status = COMMERCE_VANTIV_STATUS_REFUNDED;
      // Set the new transaction balance of the previous transaction from the same order.
      if (!empty($partial_amount)) {
        $transaction->amount = $transaction->amount - $partial_amount;
      }
      else {
        $transaction->amount = 0;
      }
      commerce_payment_transaction_save($transaction);
      $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
      if (!empty($partial_amount)) {
        $transaction->amount = $partial_amount;
      }
      else {
        $transaction->amount = $previous_transaction_amount;
      }

      $transaction->instance_id = $payment_method['instance_id'];
      $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
      //$transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $transaction->remote_id = $litle_txn_id;
      $transaction->remote_status = $message;
      $transaction->message_variables = array('@auth_code' => $auth_code);

      //Get the payload.
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
      $payload_array = $this->convertPayloadToArray($payload, $payload_attribute);
      $transaction->payload = (is_array($payload_array) == TRUE) ? $payload_array : $payload;

      if (!empty($creditResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_REFUND;
        $transaction->message .= t('<br/> Refunded:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
        drupal_set_message(t('Transaction was refunded successfully.'), 'status');
      }
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_vantiv', "This operation was not successful due to: @message", array('@message' => $message), WATCHDOG_ERROR);
          }
        }
        drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
      }
      commerce_payment_transaction_save($transaction);
      drupal_goto('admin/commerce/orders');
    }
  }

  /**
   * Process void transactions.
   */
  public function processVoidTransaction($response, $payment_method, $transaction) {
      //Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $order_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $voidResponse = litle\sdk\XmlParser::getNode($response,'voidResponse');

    if (!empty($voidResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'voidResponse', 'id');
      $payload_attribute = 'voidResponse';
    }

    //If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id)) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }

      // The captured transaction becomes voided.
      $transaction->status = COMMERCE_VANTIV_STATUS_VOIDED;
      $transaction->message .= t('<br/> Voided:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
      // Set the new transaction balance of the previous transaction from the same order.
      if (!empty($partial_amount)) {
        $transaction->amount = $transaction->amount - $partial_amount;
      }
      else {
        $transaction->amount = 0;
      }
      commerce_payment_transaction_save($transaction);
      $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
      if (!empty($partial_amount)) {
        $transaction->amount = $partial_amount;
      }
      else {
        $transaction->amount = $order->commerce_order_total['und'][0]['amount'];
      }

      $transaction->instance_id = $payment_method['instance_id'];
      $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
      //$transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $transaction->remote_id = $litle_txn_id;
      $transaction->remote_status = $message;
      $transaction->message_variables = array('@auth_code' => $auth_code);

      //Get the payload.
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
      $payload_array = $this->convertPayloadToArray($payload, $payload_attribute);
      $transaction->payload = (is_array($payload_array) == TRUE) ? $payload_array : $payload;
      if (!empty($voidResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_VOID;
        $transaction->message .= t('<br/> Void:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
        drupal_set_message(t('Transaction was voided successfully.'), 'status');
      }
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_vantiv', "This operation was not successful due to: @message", array('@message' => $message), WATCHDOG_ERROR);
          }
        }
        drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
      }
      commerce_payment_transaction_save($transaction);
      drupal_goto('admin/commerce/orders');
    }
  }

  /**
   * Processes reversed authorized transaction.
   */
  public function processReversedTransactions($response, $payment_method, $transaction, $partial_amount = '') {
    //Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $order_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $authReversalResponse = litle\sdk\XmlParser::getNode($response,'authReversalResponse');

    if (!empty($authReversalResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'authReversalResponse', 'id');
      $payload_attribute = 'authReversalResponse';
    }

    // If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id) and $order_id == $transaction->order_id) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }

      $previous_transaction = $transaction;

      // Set the new transaction balance of the previous transaction from the same order.
      if (!empty($partial_amount)) {
        $transaction->amount = $transaction->amount - $partial_amount;
      }
      else {
        $transaction->amount = 0;
        // The pending transaction becomes captured.
        $transaction->status = COMMERCE_VANTIV_STATUS_VOIDED;
      }
      // Save the old transaction.
      commerce_payment_transaction_save($transaction);

      // New transaction to save.
      $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
      if (!empty($partial_amount)) {
        $transaction->amount = $partial_amount;
      }
      else {
        $transaction->amount = $previous_transaction->amount;
      }
      $transaction->amount = 0;
      $transaction->instance_id = $payment_method['instance_id'];
      $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
      $transaction->remote_id = $litle_txn_id;
      $transaction->remote_status = $message;
      $transaction->message_variables = array('@auth_code' => $auth_code);

      // Get the payload.
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
      $payload_array = $this->convertPayloadToArray($payload, $payload_attribute);
      $transaction->payload = (is_array($payload_array) == TRUE) ? $payload_array : $payload;

      if (!empty($authReversalResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_VOID;
        $transaction->message .= t('<br/> Reversed:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
        drupal_set_message(t('Authorization reverse operation was successful.'), 'status');
      }
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_vantiv', "This operation was not successful due to: @message", array('@message' => $message), WATCHDOG_ERROR);
          }
        }
        drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
      }
      commerce_payment_transaction_save($transaction);
      drupal_goto('admin/commerce/orders');
    }
  }

  /**
   * Convert the xml payload to array.
   */
  public function convertPayloadToArray($payload, $payload_attribute) {
    $xml = simplexml_load_string($payload);
    $json = json_encode($xml);
    $json_dec = json_decode($json, TRUE);
    if (!empty($json_dec[$payload_attribute])) {
      foreach ($json_dec[$payload_attribute] as $key => $value) {
        if (strstr($key, '@') != FALSE) {
          $new_key = str_replace('@', '', $key);
          $new_array[$new_key] = $value;
        }
        else {
          $new_array[$key] = $value;
        }
      }

      foreach ($new_array['attributes'] as $k => $v) {
        if (!empty($k) and !empty($v)) {
          $atributes[$k] = $v;
        }
      }
      $new_array['attributes'] = $atributes;
      return $new_array;
    }
    else {
      return FALSE;
    }
  }
  /**
   * Get account links.
   */
  public function accountLinks($account) {
    switch ($account) {
      case 'sandbox':
        $url = 'https://www.testlitle.com/sandbox/communicator/online';
      break;
      case 'postlive':
        $url = 'https://postlive.litle.com/vap/communicator/online';
      break;
      case 'transact_postlive':
        $url = 'https://transact-postlive.litle.com/vap/communicator/online';
      break;
      case 'production':
        $url = 'https://payments.litle.com/vap/communicator/online';
      break;
      case 'production_transact':
        $url = 'https://transact.litle.com/vap/communicator/online';
      break;
      case 'prelive':
        $url = 'https://prelive.litle.com/vap/communicator/online';
      break;
      case 'transact_prelive':
        $url = 'https://transact-prelive.litle.com/vap/communicator/online';
      break;
    }
    return $url;
  }

  /**
   * Returns short payment method type.
   */
  public function cardTypes($card_type) {
    switch ($card_type) {
      case 'visa':
        $type = 'VI';
      break;
      case 'mastercard':
        $type = 'MC';
      break;
      case 'amex':
        $type = 'AX';
      break;
      case 'discover':
        $type = 'DI';
      break;
      case 'dc':
        $type = 'DC';
      break;
      case 'maestro':
        $type = 'MA';
      break;
      case 'paypal':
        $type = 'PP';
      break;
      case 'jcb':
        $type = 'JC';
      break;
      case 'bml':
        $type = 'BL';
      break;
      case 'echeck':
        $type = 'EC';
      break;
    }
    return  $type;
  }
}

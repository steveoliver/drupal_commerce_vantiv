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
    $this->transaction_type_process = !empty($settings['transaction_type_process']) ? trim($settings['transaction_type_process']) : '';
    $this->logs = !empty($settings['logs']) ? trim($settings['logs']) : '';
  }

  public function libraryLoad() {
    define('STDIN',fopen("php://stdin","r"));
    $library = libraries_info('litle');
    $load_library = libraries_load('litle');
    libraries_load_files($load_library);
  }
  public function directPayments($customer_profile, $order, $card_info, $payment_method, $amount = '') {
    $load_library = self::libraryLoad();
    $currency_code = empty($amount->currency_code) ? $order->commerce_order_total['und'][0]['currency_code'] : $amount->currency_code;
    $charge_amount = empty($amount->amount) ? $order->commerce_order_total['und'][0]['amount'] : $amount->amount;
    $card_owner_name = $customer_profile->commerce_customer_address['und'][0]['name_line'];
    $user = user_load($order->uid);
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
		      'card'=>array(
		      'number' => $card_info['credit_card']['number'],
		      'expDate' => $card_info['credit_card']['exp_month'] . substr($card_info['credit_card']['exp_year'], 2, 4),
		      'cardValidationNum' => $card_info['credit_card']['code'],
		      'type' => $this->cardTypes($card_info['credit_card']['type']))
    );

    $initilaize = new litle\sdk\LitleOnlineRequest();

    if ($this->transaction_type_process == 'pre_auth') {
      $response = $initilaize->authorizationRequest($billing_data);
    }
    elseif ($this->transaction_type_process == 'sale') {
      $response = $initilaize->saleRequest($billing_data);
    }

    /*
    if ($this->three_d_secure == 0) {
      $billing_data['orderSource'] = '3dsAttempted';
    }*/

    //Tests
    if ($this->transaction_type_process == 'sale') {
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($saleResponse);
      $response_code = litle\sdk\XmlParser::getNode($saleResponse,'response');
      variable_set('resp_code', $response_code);
      variable_set('payload', $payload);
    }

    $return = $this->processDirectTransactions($response, $payment_method);
  }

  /**
   * Process cross payments.
   */
  public function crossPayment($transaction, $partial = '', $amount = '', $type = '') {
    $load_library = self::libraryLoad();
    /*$currency_code = empty($amount->currency_code) ? $order->commerce_order_total['und'][0]['currency_code'] : $amount->currency_code;
    $charge_amount = empty($amount->amount) ? $order->commerce_order_total['und'][0]['amount'] : $amount->amount;
    $card_owner_name = $customer_profile->commerce_customer_address['und'][0]['name_line'];
    $user = user_load($order->uid);*/
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

    // Perform the http request.
    if ($type == 'capture') {
      $init = new litle\sdk\LitleOnlineRequest();
      $response = $init->captureRequest($billing_data);
    }
    elseif ($type == 'refund') {
      $init = new litle\sdk\LitleOnlineRequest();
      $response = $init->creditRequest($billing_data);
    }
    //Tests
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    variable_set('capture_response', $response_code);
    $payload = litle\sdk\XmlParser::getDomDocumentAsString($response);
    variable_set('payload_capture', $payload);
    variable_set('data', $billing_data);

    $payment_method = commerce_payment_method_instance_load($transaction->instance_id);

    // Saves partial or fully captured transaction.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'capture') {
      $return = $this->processCapturedTransactions($response, $payment_method, $transaction, $partial_amount);
    }
    elseif ($type == 'capture') {
      $return = $this->processCapturedTransactions($response, $payment_method, $transaction);
    }

    // Saves partial or fully refund transaction.
    if (!empty($partial) and $partial == TRUE and !empty($amount) and $type == 'refund') {
      $return = $this->processRefundedTransactions($response, $payment_method, $transaction, $partial_amount);
    }
    elseif ($type == 'refund') {
      $return = $this->processRefundedTransactions($response, $payment_method, $transaction);
    }
  }

//vantiv_direct|commerce_payment_vantiv_direct
  /**
   * Process the transaction according to the respond.
   */
  public function processDirectTransactions($response, $payment_method, $transaction = '') {
    //Get response information.
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
    //$captureResponse = litle\sdk\XmlParser::getNode($response,'captureResponse');

    if (!empty($authorizationResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'authorizationResponse', 'id');
      $payload_attribute = 'authorizationResponse';
    }
    if (!empty($captureResponse)) {
      $id = litle\sdk\XmlParser::getAttribute($response, 'captureResponse', 'id');
      $payload_attribute = 'captureResponse';
    }

    //If the transaction was approved.
    if ($response_code == '000') {
      if (!empty($order_id)) {
        $order = commerce_order_load($order_id);
      }
      elseif (!empty($transaction)) {
        $order = commerce_order_load($transaction->order_id);
      }
      $transaction = commerce_payment_transaction_new('vantiv_direct', $order->order_id);
      $transaction->instance_id = $payment_method['instance_id'];
      $transaction->amount = $order->commerce_order_total['und'][0]['amount'];
      $transaction->currency_code = $order->commerce_order_total['und'][0]['currency_code'];
      //$transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $transaction->remote_id = $litle_txn_id;
      $transaction->remote_status = $message;
      $transaction->message_variables = array('@auth_code' => $auth_code);

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
      }/*
      elseif (!empty($captureResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_CAPTURED;
        $transaction->message .= t('Authorized:') . t('@message: @date', array('@message' => $message, '@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
      }*/
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_ogone', "The order# @order was cancelled, due to @message", array('@order' => $order->order_id, '@message' => $message), WATCHDOG_ERROR);
          }
        }
        drupal_set_message(t('An error occur, your transaction was not processed successfully.'), 'error');
      }
      commerce_payment_transaction_save($transaction);
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
            watchdog('commerce_ogone', "The order# @order was cancelled, due to @message", array('@order' => $order->order_id, '@message' => $message), WATCHDOG_ERROR);
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
      variable_set('payload_refund', $payload);
      if (!empty($creditResponse)) {
        $transaction->status = COMMERCE_VANTIV_STATUS_REFUND;
        $transaction->message .= t('<br/> Refunded:') . t('@date', array('@date' => format_date(REQUEST_TIME, 'short')));
        $transaction->data = array('id' => $id);
        drupal_set_message(t('Transaction was refunded successfully.'), 'status');
      }
      else {
        if (!empty($message) and !empty($order)) {
          if ($this->logs == 0) {
            watchdog('commerce_ogone', "The order# @order was cancelled, due to @message", array('@order' => $order->order_id, '@message' => $message), WATCHDOG_ERROR);
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
      dsm($new_array, '$new_array');
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

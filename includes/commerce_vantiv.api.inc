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
  }

  public function library_load() {
    define('STDIN',fopen("php://stdin","r"));
    $library = libraries_info('litle');
    $load_library = libraries_load('litle');
    libraries_load_files($load_library);
  }
  public function directPayments($customer_profile, $order, $card_info, $payment_method, $amount = '') {
    $load_library = self::library_load();
    $currency_code = empty($amount->currency_code) ? $order->commerce_order_total['und'][0]['currency_code'] : $amount->currency_code;
    $charge_amount = empty($amount->amount) ? $order->commerce_order_total['und'][0]['amount'] : $amount->amount;
    $card_owner_name = $customer_profile->commerce_customer_address['und'][0]['name_line'];
    $user = user_load($order->uid);
    $billing_data = array(
          'user' => $this->user,
          'password' => $this->password,
          'merchantId' => $this->merchant_id,
          'reportGroup' => 'rtpGrp',
          'url' => $this->account_links($this->account) ,
          'litle_requests_path' => $this->account_links($this->account)  ,
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
          'id'=> $user->uid,
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
      $saleResponse = $initilaize->authorizationRequest($billing_data);
    }
    elseif ($this->transaction_type_process == 'sale') {
      $saleResponse = $initilaize->saleRequest($billing_data);
    }

/*
    if ($this->three_d_secure == 0) {
      $billing_data['orderSource'] = '3dsAttempted';
    }*/




    if ($this->transaction_type_process == 'sale') {
      $payload = litle\sdk\XmlParser::getDomDocumentAsString($saleResponse);
      $response_code = litle\sdk\XmlParser::getNode($saleResponse,'response');


      variable_set('resp_code', $response_code);
      variable_set('payload', $payload);
    }

    $return = $this->process_transaction($saleResponse, $payment_method);
  }
//vantiv_direct|commerce_payment_vantiv_direct
  /**
   * Process the transaction according to the respond.
   */
  public function process_transaction($saleResponse, $payment_method) {
      //Get response information.
    $response_code = litle\sdk\XmlParser::getNode($response,'response');
    $oder_id = litle\sdk\XmlParser::getNode($response,'orderId');
    $response_time = litle\sdk\XmlParser::getNode($response,'responseTime');
    $message = litle\sdk\XmlParser::getNode($response,'message');
    $auth_code = litle\sdk\XmlParser::getNode($response,'authCode');
    $avs_result = litle\sdk\XmlParser::getNode($response,'avsResult');
    $card_validation_result = litle\sdk\XmlParser::getNode($response,'cardValidationResult');
    $litle_txn_id = litle\sdk\XmlParser::getNode($response,'litleTxnId');

    $authorizationResponse = litle\sdk\XmlParser::getNode($response,'authorizationResponse');
    $saleResponse = litle\sdk\XmlParser::getNode($response,'saleResponse');

    //If the transaction was approved.
    if ($response_code == '000') {
      $order = commerce_order_load($oder_id);
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
        $transaction->payload = $payload;

      if (!empty($authorizationResponse) /*and $this->transaction_type_process == 'pre_auth'*/) {
        $transaction->status = COMMERCE_PAYMENT_STATUS_PENDING;
        $transaction->message .= t('Authorized:') . t('@message: @date', array('@message' => $message, '@date' => format_date(REQUEST_TIME, 'short')));
      }
      elseif (!empty($saleResponse) /*and $this->transaction_type_process == 'sale'*/) {
        $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
        $transaction->message .= t('Sale:') . t('@message: @date', array('@message' => $message, '@date' => format_date(REQUEST_TIME, 'short')));
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
    }
  }

  /**
   * Get account links.
   */
  public function account_links($account) {
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

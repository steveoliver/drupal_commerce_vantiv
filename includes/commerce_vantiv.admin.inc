<?php
/**
 * @file
 * Provide admin functionalities such as manual capture, refund, cancel and delete transactions.
 */

/**
 * Capture transactions.
 */
function commerce_vantiv_capture_form($form, &$form_state, $order, $transaction) {
  $form_state['order'] = $order;
  $form_state['transaction'] = $transaction;

  // Load and store the payment method instance for this transaction.
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['payment_method'] = $payment_method;

  $balance = commerce_payment_order_balance($order);

  if ($balance['amount'] > 0 && $balance['amount'] < $transaction->amount) {
    $default_amount = $balance['amount'];
  }
  else {
    $default_amount = $transaction->amount;
  }

  $default_amount = number_format(commerce_currency_amount_to_decimal($default_amount, $transaction->currency_code), 2, '.', '');

  $description =
    t('Authorization: @amount,', array('@amount' => commerce_currency_format($transaction->amount, $transaction->currency_code))) .
    t('Order balance: @balance', array('@balance' => commerce_currency_format($balance['amount'], $balance['currency_code'])));

  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Capture amount'),
    '#description' => t('@description', array('@description' => $description)),
    '#default_value' => $default_amount,
    '#field_suffix' => check_plain($transaction->currency_code),
    '#size' => 16,
  );

  $form = confirm_form($form,
      t('What amount do you want to capture?'),
      'admin/commerce/orders/' . $order->order_id . '/payment',
      '',
      t('Capture'),
      t('Cancel'),
      'confirm'
  );

  return $form;
}

/**
 * Validate handler: ensure a valid amount is given.
 */
function commerce_vantiv_capture_form_validate($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $amount = $form_state['values']['amount'];
  // Ensure a positive numeric amount has been entered for capture.
  if (!is_numeric($amount) || $amount <= 0) {
    form_set_error('amount', t('You must specify a positive numeric amount to capture.'));
  }

  // Ensure the amount is less than or equal to the authorization amount.
  if ($amount > commerce_currency_amount_to_decimal($transaction->amount, $transaction->currency_code)) {
    form_set_error('amount', t('You cannot capture more than you authorized through Ogone.'));
  }
}

/**
 * Submit handler: process a prior authorization capture via Vantiv.
 */
function commerce_vantiv_capture_form_submit($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $entered_amount = commerce_currency_decimal_to_amount($form_state['values']['amount'], $form_state['build_info']['args'][0]->commerce_order_total['und'][0]['currency_code']);

  /*$order = commerce_order_load($transaction->order_id);
  $payment_methods = commerce_payment_method_instance_load($transaction->instance_id);

  $amount = commerce_currency_decimal_to_amount($form_state['values']['amount'], $transaction->currency_code);
  $description = t('Authorization capture for order #@order_id at @site_name', array(
    '@order_id' => $order->order_id,
    '@site_name' => variable_get('site_name', ''),
  ));*/

  // Check if partial transaction is required.
  if ($form_state['build_info']['args'][0]->commerce_order_total['und'][0]['amount'] > $entered_amount) {
    $partial_capture = TRUE;
    $amount = $entered_amount;
  }
  $api = commerce_vantiv_api_object();
  $result = $api->crossPayment($transaction);


}









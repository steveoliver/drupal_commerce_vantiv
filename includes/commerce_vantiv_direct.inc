<?php
/**
 * @file
 * This file implements the direct payment functionality for
 * Little and co payment gateway.
 * Payment method instance id: vantiv_direct|commerce_payment_vantiv_direct
 */

/**
 * Submit callback for commerce_vantiv.
 */
function commerce_vantiv_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
    $cardonfile_capable = module_exists('commerce_cardonfile') && !empty($payment_method['settings']['cardonfile']);
    module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
    $card_types = commerce_vantiv_direct_all_card_names();
    foreach ($card_types as $key => $value) {
      $cards_id[] = $key;
    }
    $form = commerce_payment_credit_card_form();
    $credit_card_settings = array(
      'type'       => $cards_id,
      'code'       => 'CVV/CV2',
      'owner' => '',
    );


    $form = commerce_payment_credit_card_form($credit_card_settings);
    $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
    $form['credit_card']['type']['#options'] = $card_types;

      if ($cardonfile_capable) {
        $storage = variable_get('commerce_cardonfile_storage', 'opt-in');
        if (in_array($storage, array('opt-in', 'opt-out')) && !$payment_method['settings']['continuous']) {
          /*$form['credit_card']['cardonfile_store'] = array(
            '#type' => 'checkbox',
            '#title' => t('Store this credit card on file for future use.'),
            '#default_value' => $storage == 'opt-out',
          );*/
        }
        else {
          $form['credit_card']['cardonfile_store'] = array(
            '#type' => 'hidden',
            '#value' => TRUE,
          );
        }

        $form['cardonfile_instance_default'] = array(
          '#type' => 'checkbox',
          '#title' => t('Set as your default card'),
          //'#default_value' => $instance_default_default_value || $force_instance_default,
          //'#access' => !$instance_default_default_value,
          //'#disabled' => $force_instance_default,
          '#states' => array(
            'invisible' => array(
              ':input[name$="[cardonfile]"]' => array('value' => 'new'),
            ),
            'visible' => array(
              ':input[name$="[cardonfile_store]"]' => array('checked' => TRUE),
            ),
          ),
        );
      }


    return $form;
}

/**
 * Get all card types.
 */
function commerce_vantiv_direct_all_card_names() {
  return array(
    'visa'    => t('Visa'),
    'mastercard'      => t('Mastercard'),
    'amex'    => t('American Express'),
    'maestro' => t('Maestro'),
    'paypal' => t('PayPal'),
    'jcb' => t('JCB'),
    'bml' => t('Bill Me Later'),
    'echeck' => t('eCheck'),
  );
}

/**
 * Validates the cardholder data.
 */
function commerce_vantiv_submit_form_validate($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  dsm($pane_values, '$pane_values');
  if (empty($pane_values['credit_card']['number']) and empty($pane_values['cardonfile'])) {
    form_set_error('number');
    return FALSE;
  }
  dsm($payment_method, '$payment_method');
  dsm($order, '$order');
  $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
  dsm($customer_profile, '$customer_profile');
  //return false;
}

/**
 * Performs http request.
 */
function commerce_vantiv_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  $api = commerce_vantiv_api_object();
  $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
  if(!empty($pane_values['credit_card']['cardonfile_store']) and $pane_values['credit_card']['cardonfile_store'] == TRUE) {
    $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method);
  }
  elseif(!empty($pane_values['cardonfile']) and is_numeric($pane_values['cardonfile']) == TRUE) {
    // Load the card.
    $card = commerce_cardonfile_load($pane_values['cardonfile']);
    $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', $card->remote_id);
  }
  else {
    $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method);
  }
  return $result;
}



<?php
/**
 * @file
 * This file implements the direct payment functionality for
 * Little and co payment gateway.
 * Payment method instance id: vantiv_direct|commerce_payment_vantiv_direct
 */

/*
function commerce_vantiv_commerce_payment_credit_card_form_alter(&$form, &$form_state) {
  dsm($form_state, '$payment form form_state hook_TYPE_alter');
  dsm($form, '$payment_form form hook_TYPE_alter');
}
*/

function commerce_vantiv_commerce_vantiv_submit_form_alter(&$form, &$form_state) {
  /*dsm($form_state, '$form_state hook_TYPE_alter');
  dsm($form, '$form hook_TYPE_alter');*/
}

/**
 * Alters the checkout payment form.
 */
function commerce_vantiv_commerce_cardonfile_checkout_pane_form_alter(&$form, &$form_state) {
  if ($form_state['commerce_payment']['payment_method']['#default_value'] == 'vantiv_echeck|commerce_payment_vantiv_echeck') {
    // Change the label for card storage.
    $form['credit_card']['cardonfile_store']['#title'] = t('Store this check on file for further use.');
    if(!empty($form['cardonfile']['#options'])) {
      foreach ($form['cardonfile']['#options'] as $key => $value) {
        if (is_numeric($key) == TRUE) {
          $new_value = str_replace('Card', 'Account', $value);
          $piece = explode(',', t('@str', array('@str' => $new_value)));
          $new_value_replace = $piece[0];
          $new_array[$key] = $new_value_replace;
        }
      }
      $new_array['new'] = t('Use a different credit card.');
      $form['cardonfile']['#options']  = $new_array;
      $form['cardonfile']['#default_value'] = 'new';
    }
  }
  //dsm($form, 'form altered');
}

/**
 * Submit callback for commerce_vantiv.
 */
function commerce_vantiv_submit_form($payment_method, $pane_values, $checkout_pane, $order) {
  $cardonfile_capable = module_exists('commerce_cardonfile') && !empty($payment_method['settings']['cardonfile']);
  if ($payment_method['method_id'] == 'vantiv_direct') {

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
          $form['credit_card']['cardonfile_store'] = array(
            '#type' => 'checkbox',
            '#title' => t('Store this credit card on file for future use.'),
            '#default_value' => $storage == 'opt-out',
          );
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

   /*$session_id =  'ASDFG-AXXXXAB999';
   $form['hidden_fraud_kit'] = array(
     '#type' => 'hidden',
     '#title' => t('Hidden element.'),
     '#prefix' => '<!-Begin ThreatMetrix profiling tags below -->
        <!- note: replace "UNIQUE_SESSION_ID" with a uniquely generated handle
        note: for production, replace "h.online-metrix.net" with a local URL and
        configure your web server to redirect to "h.online-metrix.net" -->
        <div style="background:url(http://v-ivanov.com/projects/drupal_test/vantiv_1/fp/clear.png?org_id=
        q5ua9sl2&amp;session_id='.$session_id.'&amp;m=1)"> </div>
        <img src="http://v-ivanov.com/projects/drupal_test/vantiv_1/fp/clear.png?org_id=q5ua9sl2&amp;
        session_id='.$session_id.'&amp;m=2" />
        <script src="http://v-ivanov.com/projects/drupal_test/vantiv_1/fp/check.js?org_id=q5ua9sl2&amp;
        session_id='.$session_id.'&amp;pageid=##PAGEID##">
        </script>
        <object type="application/x-shockwave-flash" data="http://v-ivanov.com/projects/drupal_test/vantiv_1/fp/fp.swf?org_id=q5ua9sl2&amp;session_id='.$session_id.'" width="1"
        height="1">
        <param name="movie" value="http://v-ivanov.com/projects/drupal_test/vantiv_1/fp/fp.swf?org_id=q5ua9sl2&amp;session_id='.$session_id.'" />
        <param name="wmode" value="transparent" />
        <div></div>
        </object>
        <!- End profiling tags -->',
     '#default_value' => $session_id,
     '#value' => $session_id,
   );*/
  }
  elseif ($payment_method['method_id'] == 'vantiv_echeck') {
    $form['#attached']['css'] = array(
      drupal_get_path('module', 'commerce_vantiv') . '/css/style_vativ.css',
    );
    $form['accType'] = array(
      '#type' => 'select',
      '#title' => t('Account type') . ' <span class="star_required">*</span>',
      '#options' => array(
        'Checking' => t('Checking'),
        'Savings'  => t('Savings'),
        'Corporate'  => t('Corporate'),
        'Corp Savings'  => t('Corp Savings'),
      ),
      /*'#states' => array(
        'visible' => array(
          ':input[name$="[cardonfile]"]' => array('value' => 'new'),
        ),
        'invisible' => array(
          ':input[name$="[cardonfile_store_card]"]' => array('checked' => TRUE),
        ),
      ),*/
    );

    $form['accNum'] = array(
      '#type' => 'textfield',
      '#title' => t('Account number') . ' <span class="star_required">*</span>',
      '#default_value' => '',
      //'#required' => TRUE,
      '#attributes' => array('autocomplete' => 'off'),
      '#maxlength' => 17,
      '#size' => 17,
      '#states' => array(
        'visible' => array(
          ':input[name$="[cardonfile]"]' => array('value' => 'new'),
        ),
        'invisible' => array(
          ':input[name$="[cardonfile_store_card]"]' => array('checked' => TRUE),
        ),
      ),
    );

    $form['routingNum'] = array(
      '#type' => 'textfield',
      '#title' => t('Routing number') . ' <span class="star_required">*</span>',
      '#default_value' => '',
      //'#required' => TRUE,
      '#attributes' => array('autocomplete' => 'off'),
      '#maxlength' => 9,
      '#size' => 9,
      /*'#states' => array(
        'visible' => array(
          ':input[name$="[cardonfile]"]' => array('value' => 'new'),
        ),
        'invisible' => array(
          ':input[name$="[cardonfile_store_card]"]' => array('checked' => TRUE),
        ),
      ),*/
    );

    $form['checkNum'] = array(
      '#type' => 'textfield',
      '#title' => t('Check number') . ' <span class="star_required">*</span>',
      '#default_value' => '',
      '#attributes' => array('autocomplete' => 'off'),
      //'#required' => TRUE,
      '#maxlength' => 15,
      '#size' => 15,
      /*'#states' => array(
        'visible' => array(
          ':input[name$="[cardonfile]"]' => array('value' => 'new'),
        ),
        'invisible' => array(
          ':input[name$="[cardonfile_store_card]"]' => array('checked' => TRUE),
        ),
      ),*/
    );

      $form['cardonfile_instance_default']['#states'] = array(
        'invisible' => array(
          ':input[name$="[cardonfile]"]' => array('value' => 'new'),
        ),
        'visible' => array(
          ':input[name$="[cardonfile_store_card]"]' => array('checked' => TRUE),
        ),
      );

    if ($cardonfile_capable) {
      $storage = variable_get('commerce_cardonfile_storage', 'opt-in');
      if (in_array($storage, array('opt-in', 'opt-out')) && !$payment_method['settings']['continuous']) {
        $form['credit_card']['cardonfile_store'] = array(
          '#type' => 'checkbox',
          '#title' => t('Store this credit card on file for future use.'),
          '#default_value' => $storage == 'opt-out',
        );
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

  }

  drupal_alter('commerce_vantiv_submit_form', $form, $form_state['values'] = $payment_method);

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

 /*     form_error($pane_form['accNum'], 'The account number should be longer than 4 characters.');
      return false;*/
  if ($payment_method['method_id'] == 'vantiv_direct') {
    if (empty($pane_values['credit_card']['number']) and empty($pane_values['cardonfile'])) {
      form_set_error('number');
      return FALSE;
    }
    $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
  }
  //return false;
  elseif ($payment_method['method_id'] == 'vantiv_echeck') {

    if (strlen($pane_values['accNum']) < 4 and (!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new')) {
      form_error($pane_form['accNum'], 'The account number should be longer than 4 characters.');
      return false;
    }
    elseif (strlen($pane_values['routingNum']) < 9 and (!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new')) {
      form_error($pane_form['routingNum'], 'The account number should be equal to 9 characters.');
      return false;
    }

    // Check if account number, routing number and check number are entered.
    if ((!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new') and empty($pane_values['accType'])) {
      form_error($pane_values['accType'], 'The account type should not be empty.');
      drupal_set_message(t('Account type is required field'), 'error');
      return FALSE;
    }
    if ((!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new') and empty($pane_values['accNum'])) {
      form_error($pane_values['accNum'], 'The account number should not be empty.');
      drupal_set_message(t('Account number is required field'), 'error');
      return FALSE;
    }
    if ((!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new') and empty($pane_values['routingNum'])) {
      form_error($pane_values['routingNum'], 'The routing number should not be empty.');
      drupal_set_message(t('Routing number is required field'), 'error');
      return FALSE;
    }
    if ((!empty($pane_values['cardonfile']) and $pane_values['cardonfile'] == 'new') and empty($pane_values['checkNum'])) {
      form_error($pane_values['checkNum'], 'The check number should not be empty.');
      drupal_set_message(t('Check number is required field'), 'error');
      return FALSE;
    }
  }

}

/**
 * Performs http request.
 */
function commerce_vantiv_submit_form_submit($payment_method, $pane_form, $pane_values, $order, $form_parents = array()) {
  $api = commerce_vantiv_api_object();
  $customer_profile = commerce_customer_profile_load($order->commerce_customer_billing['und'][0]['profile_id']);
  // Direct transaction without using card on file.
  if ($payment_method['method_id'] == 'vantiv_direct') {
    if(!empty($pane_values['credit_card']['cardonfile_store']) and $pane_values['credit_card']['cardonfile_store'] == TRUE) {

      if (!empty($pane_values['hidden_fraud_kit'])) {
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', $pane_values['hidden_fraud_kit']);
      }
      else {
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method);
      }
    }
    elseif(!empty($pane_values['cardonfile']) and is_numeric($pane_values['cardonfile']) == TRUE) {
      // Load the card.

      $card = commerce_cardonfile_load($pane_values['cardonfile']);
      $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', $card->remote_id);
    }
    else {
      //$result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method);
      if (!empty($pane_values['hidden_fraud_kit'])) {

        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', $pane_values['hidden_fraud_kit']);
      }
      else {
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method);
      }
    }
  }
  elseif ($payment_method['method_id'] == 'vantiv_echeck') {
    // Saves a new account.
    if(!empty($pane_values['credit_card']['cardonfile_store']) and $pane_values['credit_card']['cardonfile_store'] == TRUE) {
      /*if (!empty($pane_values['hidden_fraud_kit'])) {
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', $pane_values['hidden_fraud_kit']);
      }
      else {*/
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', '', TRUE);
     // }
    }
    // Use already saved account.
    elseif(!empty($pane_values['cardonfile']) and is_numeric($pane_values['cardonfile']) == TRUE) {
      // Load the card.

      $card = commerce_cardonfile_load($pane_values['cardonfile']);
      $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', $card->remote_id, '', TRUE);
    }
    else {
      // No saved accounts to be used, just process the transaction.
      /*if (!empty($pane_values['hidden_fraud_kit'])) {
        variable_set('direct', '1');
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', $pane_values['hidden_fraud_kit']);
      }
      else {*/
        $result = $api->directPayments($customer_profile, $order, $pane_values, $payment_method, '', '', '', TRUE);
      //}
    }

  }
  return $result;
}


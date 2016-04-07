<?php
/**
 * @file
 */
 
/**
 * Implements the Form API.
 * Provides a form for users to enter data and generate a PCoA plot.
 */
function user_charts_pcoa_form($form, &$form_state) {
  
  // Add the javascript libraries needed for this chart.
  $form['#attached']['js'] = array(
    'http://d3js.org/d3.v3.min.js',
    'http://labratrevenge.com/d3-tip/javascripts/d3.tip.v0.6.3.js',
    drupal_get_path('module','user_charts') . '/js/pcoaScatterplot.chart.js',
  );
  libraries_load('saveSvgAsPng');
  
  // Also add the css.
  $form['#attached']['css'] = array(
    drupal_get_path('module','user_charts') . '/css/pcoa_scatterplot.chart.css',
  );
  
  // DEFAULTS ///////////////
  $defaults = array(
    'pcoa' => '',
    'grouping' => ''
  );
  if (isset($form_state['values']['pcoa_results']) AND !empty($form_state['values']['pcoa_results'])) {
    $defaults['pcoa'] = $form_state['values']['pcoa_results'];
  }
  if (isset($form_state['input']['pcoa_results']) AND !empty($form_state['input']['pcoa_results'])) {
    $defaults['pcoa'] = $form_state['input']['pcoa_results'];
  }
  if (isset($form_state['values']['grouping'])) {
    $defaults['grouping'] = $form_state['values']['grouping'];
  }
  if (isset($form_state['input']['grouping']) AND !empty($form_state['input']['grouping'])) {
    $defaults['grouping'] = $form_state['input']['grouping'];
  }

  // DATA ///////////////////
  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #1: Data'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? TRUE : FALSE,
  );
  
  $form['data']['pcoa_results'] = array(
    '#type' => 'textarea',
    '#title' => 'PCoA Results',
    '#description' => 'A <strong>tab-delimited</strong> dataset containing at least three columns: the 
      name of your individuals and the first two principal component numbers (any additional columns will be ignored). 
      <strong>Do not include a header.</strong>',
    '#required' => TRUE,
    '#value' => $defaults['pcoa'],
  );
  
  $form['data']['grouping'] = array(
    '#type' => 'textarea',
    '#title' => 'Grouping List',
    '#description' => 'A <strong>tab-delimited listing of all individuals</strong> (first column) and the 
      group (ie: coutry, ecological zone, etc.) they are in. Groups will be used to colour 
      code your graph. <strong>Do not include a header.</strong> Furthermore, at most 20 groups are 
      supported (if you have more than 20 groups then the smallest groups will be merged 
      into an "Other" category.',
    '#value' => $defaults['grouping'],
  );

  // EXAMPLE ////////////////
  $form['data']['example'] = array(
    '#type' => 'fieldset',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#title' => 'Example'
  );
  
  // Copy for elements for the example.
  $form['data']['example']['pcoa_example'] = $form['data']['pcoa_results'];
  $form['data']['example']['grouping_example'] = $form['data']['grouping'];
  
  // Assign the example data to the example fields.
  $example_data = user_charts_pcoa_example_data();
  $form['data']['example']['pcoa_example']['#value'] = $example_data['pcoa'];
  $form['data']['example']['grouping_example']['#value'] = $example_data['grouping'];
  
  // Make these fields disabled so users don't edit my examples ;-).
  $form['data']['example']['pcoa_example']['#disabled'] = TRUE;
  $form['data']['example']['grouping_example']['#disabled'] = TRUE;
  
  // Save Example for use later.
  $form_state['example'] = $example_data;
  
  // Now give them a button to render it.
  $form['data']['example']['show_example'] = array(
    '#type' => 'submit',
    '#value' => 'Show Example',
    '#attributes' => array('class' => array('example-button')),
    '#submit' => array('user_charts_pcoa_form_submit'),
    '#limit_validation_errors' => array(array('example')),
  );
  
  // CHART //////////////////
  $form['chart'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #2: Visualization'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? FALSE : TRUE,
  );
  
  if (isset($form_state['show_example']) AND $form_state['show_example']) {
    $form['chart']['msg'] = array(
      '#type' => 'markup',
      '#prefix' => '<div class="messages warning">',
      '#suffix' => '</div>',
      '#markup' => 'The following chart is showing EXAMPLE DATA. To show your own data, click "Generate" at the bottom of this form.',
    );
  }
  
  $form['chart']['placeholder'] = array(
    '#type' => 'markup',
    '#markup' => '<span id="chart"></span>',
  );
  
  /*
  $form['config'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #3: Configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? FALSE : TRUE,
  );
  */
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Generate',
  );

  if (isset($form_state['values'])) {
    $form['download'] = array(
      '#type' => 'submit',
      '#prefix' => '<div id="download-svg-link">',
      '#value' => 'Save the chart as PNG',
      '#suffix' => '</div>',
    );
  }
  
  return $form;
}

/**
 * Submit form function following the Drupal Form API.
 */
function user_charts_pcoa_form_validate($form, &$form_state) {
    
  // If they pressed the "Show Example" button then use the example data.
  if ($form_state['triggering_element']['#value'] == 'Show Example') {
    $pcoa_results = $form_state['example']['pcoa'];
    $grouping = $form_state['example']['grouping'];
    $form_state['show_example'] = TRUE;
  }
  else {
    $pcoa_results = $form_state['input']['pcoa_results'];
    $grouping = $form_state['input']['grouping'];
    $form_state['show_example'] = FALSE;
  }
  
  // We want to save our data for use in the submit...
  // Initialize a place to do so.
  $form_state['processed'] = array(
    'pcoa_results' => array(), 
    'pcoa_mapping' => array(),
    'groups' => array(),
    'unique_groups' => array(
      'counts' => array(),
      'indiv' => array(),
    ),
    'group_mapping' => array(),
  );

  // Validate the grouping if present.
  // We do this before the results in order to ensure all individuals have a grouping.
  if (!empty($grouping)) {
    $i = 0;
    $groups = explode(PHP_EOL, $grouping);
    foreach($groups as $k => $g) {
      $i++;
      if (!empty($g)) {
        $tmp = preg_split('/(\t|\s\s+)/',$g);
      
        // There must be two columns.
        if (sizeof($tmp) < 2) {
          form_set_error('grouping', 'Row '.$i.' ("'.$g.'") must contain 2 columns. Currently columns are interpreted as: "'.implode('", "',$tmp).'". Make sure your data is separated by a tab or 2+ spaces.');
        }
        else {
          $form_state['processed']['groups'][] = array(
            'line' => $tmp[0],
            'group' => $tmp[1],
          );
          $curr_group = $tmp[1];
        
          // Magic related to only keeping the top 20 groups.
          if (!isset($form_state['processed']['unique_groups']['counts'][$curr_group])) {
            $form_state['processed']['unique_groups']['counts'][$curr_group] = 0;
            $form_state['processed']['unique_groups']['indiv'][$curr_group] = array();
          }
          $form_state['processed']['unique_groups']['counts'][$curr_group]++;
          $form_state['processed']['unique_groups']['indiv'][$curr_group][] = $tmp[0];
          $form_state['processed']['group_mapping'][  $tmp[0] ] = $i-1;
        }
      }
    }
  }
  // Otherwise we want to assign all individuals to the same group.
  else {
    $form_state['processed']['unique_groups']['counts']['Uknown'] = sizeof($form_state['processed']['pcoa_results']);
    $form_state['processed']['unique_groups']['indiv']['Uknown'] = array();
    $i = 0;
    foreach ($form_state['processed']['pcoa_results'] as $r) {
      $i++;
      $form_state['processed']['groups'][] = array(
        'line' => $r['name'],
        'group' => 'Unknown'
      );
      $form_state['processed']['unique_groups']['indiv']['Uknown'][] = $r['name'];
      $form_state['processed']['group_mapping'][  $r['name'] ] = $i-1;
    }
  }


  // Validate that the PCoA Results is tab/multi-space delimited
  //   and contains at least 3 columns.
  $results = explode(PHP_EOL,$pcoa_results);
  $i = 0;
  foreach($results as $k => $r) {
    $i++;
    if (!empty($r)) {
      $tmp = preg_split('/(\t|\s\s+)/',$r);
      if (sizeof($tmp) < 3) {
        form_set_error('pcoa_results', 'Row '.$i.' ("'.$r.'") must contain 3 columns. Currently columns are interpreted as: "'.implode('", "',$tmp).'". Make sure your data is separated by a tab or 2+ spaces.');
      }
      // Also check if columns 2 & 3 are numeric.
      else {
      
        $tmp[1] = trim($tmp[1]);
        $tmp[2] = trim($tmp[2]);
      
        if (!is_numeric($tmp[1])) {
          form_set_error('pcoa_results', 'Column 2 ('.$tmp[1].') of row '.$i.' ("'.implode('", "',$tmp).'") must be a number.');
        }
        elseif (!is_numeric($tmp[2])) {
          form_set_error('pcoa_results', 'Column 3 ('.$tmp[2].') of row '.$i.' ("'.implode('", "',$tmp).'") must be a number.');
        }
        // If we get to this point then the data is good -save it so we don't have to re-process in submit.
        else {
          $form_state['processed']['pcoa_results'][] = array(
            'name' => $tmp[0],
            'p1' => $tmp[1],
            'p2' => $tmp[2],
          );
          $form_state['processed']['pcoa_mapping'][ $tmp[0] ] = $i-1;
          
          // Check if this individual has a group and if not, assign "Uknown".
          if (!isset($form_state['processed']['group_mapping'][ $tmp[0] ])) {
            if (!empty($grouping)) drupal_set_message('"'.$tmp[0].'" was missing from the grouping list and, as such, was assigned a group of "Uknown".','warning');
            $form_state['processed']['groups'][] = array(
              'line' => $tmp[0],
              'group' => 'Unknown'
            );
            if (!isset($form_state['processed']['unique_groups']['counts']['Uknown'])) $form_state['processed']['unique_groups']['counts']['Uknown'] = 0;
            if (!isset($form_state['processed']['unique_groups']['indiv']['Uknown'])) $form_state['processed']['unique_groups']['indiv']['Uknown'] = array();
            $form_state['processed']['unique_groups']['counts']['Uknown']++;
            $form_state['processed']['unique_groups']['indiv']['Uknown'][] = $tmp[0];
            
          }
        }
      }
    }
  }
  
}

/** 
 *
 */
function user_charts_pcoa_form_submit($form, &$form_state) {
  
  $form_state["rebuild"] = TRUE;
  
  $form_state['values']['pcoa_results'] = $form_state['input']['pcoa_results'];
  $form_state['values']['grouping'] = $form_state['input']['grouping'];
  
  // Processing was done in validate for consistency & efficiency.
    
  // We can only support 20 groups due to the number of distiguisable colours available to d3.js
  // If we have more than 20 groups we intend to combine the groups with the least number
  // of individuals until we reach a total of 21.
  $total_count = sizeof($form_state['processed']['unique_groups']['counts']);
  
  if ($total_count > 20) {
    drupal_set_message('This chart is limited to 20 groups but you have ' . $total_count . '. We have merged your smallest ' . ($total_count - 20) . ' groups into a single group labelled "Other" to adhere to this restriction.','warning');
  }

  arsort($form_state['processed']['unique_groups']['counts']);
  $i = 0;
  $sorted_results = array();
  foreach ($form_state['processed']['unique_groups']['counts'] as $group_name => $count) {
    $i++;
    foreach ($form_state['processed']['unique_groups']['indiv'][$group_name] as $indiv) {
      // After we've exceeded 20 groups, merge remaining groups into "Other"
      if ($i > 20) {
        $k = $form_state['processed']['group_mapping'][$indiv];
        $groups[$k]['group'] = 'Other';
      }

      if (isset($form_state['processed']['pcoa_mapping'][$indiv])) {
        $k = $form_state['processed']['pcoa_mapping'][$indiv];
        $sorted_results[] = $form_state['processed']['pcoa_results'][$k];    
      }
    }
  }
  
  // In the case where groups were nor supplied, results will not be sorted above.
  // Thus we will just keep the original ordering.
  if (empty($sorted_results)) $sorted_results = $form_state['processed']['pcoa_results'];
  
  // We would also like to sort the result set to match the group order. This would ensure
  // that the legend matches the group order.
  
  // Now pass both to the javascript chart as settings.
  $settings = array(
    'PCoA' => $sorted_results,
    'groups' => $form_state['processed']['groups'],
  );
  drupal_add_js(array('userCharts' => $settings), 'setting');

  // Save the values used to build the chart.
  $form_state['user_charts'] = $settings;
}

/**
 * Provide example input to guide users with difficulty understanding the format
 * and to demonstrate functionality to those without data handy.
 */
function user_charts_pcoa_example_data() {
  $path = drupal_get_path('module', 'user_charts') . '/example_data/';
  $pcoa = $path . 'PCoA_results.tsv';
  $grouping = $path . 'PCoA_grouping.tsv';
  return array(
    'pcoa' => file_get_contents(drupal_realpath($pcoa)),
    'grouping' => file_get_contents(drupal_realpath($grouping)),
  );
}

<?php
/**
 * @file
 */
 
/**
 * Implements the Form API.
 * Provides a form for users to enter data and generate a PCoA plot.
 */
function user_charts_pcoa_form($form, $form_state) {

  $form['msg'] = array(
    '#type' => 'markup',
    '#markup' => '<span style="font-size:1.1em;color:#CFC045;">This tool is in <strong>beta (early stages of development)</strong> and as such 
    there is not yet robust error checking. <strong>If you run into problems, please contact your 
    administrator, and as always, double check your results.</strong></span>'
  );
  
  // Add the javascript libraries needed for this chart.
  $form['#attached']['js'] = array(
    'http://d3js.org/d3.v3.min.js',
    'http://labratrevenge.com/d3-tip/javascripts/d3.tip.v0.6.3.js',
    drupal_get_path('module','user_charts') . '/js/pcoaScatterplot.chart.js',
  );
  // Also add the css.
  $form['#attached']['css'] = array(
    drupal_get_path('module','user_charts') . '/css/pcoa_scatterplot.chart.css',
  );

  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #1: Data'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? TRUE : FALSE,
  );
  
  $form['data']['pcoa_results'] = array(
    '#type' => 'textarea',
    '#title' => 'PCoA Results',
    '#description' => 'A tab-delimited dataset containing at least three columns: the 
      name of your individuals and the first two principal component numbers. 
      Do not include a header and names should not contain spaces.',
    '#required' => TRUE,
  );
  
  $form['data']['grouping'] = array(
    '#type' => 'textarea',
    '#title' => 'Grouping',
    '#description' => 'A tab-delimited listing of all individuals (first column) and the 
      group (ie: coutry, ecological zone, etc.) they are in. Groups will be used to colour 
      code your graph and should not contain spaces. Furthermore, at most 20 groups are 
      supported (if you have more than 20 groups then the smallest groups will be merged 
      into an "Other" category.',
  );

  $form['chart'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #2: Visualization'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? FALSE : TRUE,
  );
  
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
  
  return $form;
}

/** 
 *
 */
function user_charts_pcoa_form_submit($form, &$form_state) {
  
  $form_state["rebuild"] = TRUE;
  
  // First process the PCoA Results.
  $indiv_mapping_results = array();
  if (!empty($form_state['values']['pcoa_results'])) {
    $results = explode(PHP_EOL,$form_state['values']['pcoa_results']);
    foreach($results as $k => $r) {
      $tmp = preg_split('/\s+/',$r);
      if (!empty($tmp[0])) {
        $results[$k] = array();
        $results[$k]['name'] = $tmp[0];
        $results[$k]['p1'] = $tmp[1];
        $results[$k]['p2'] = $tmp[2];

        $indiv_mapping_results[$tmp[0]] = $k;
      }
      else {
        unset($results[$k]);
      }
    }
  }
  
  // Now process the groupings.
  $uniq_groups_with_counts = array();
  $uniq_groups_indiv = array();
  $indiv_mapping = array();
  if (!empty($form_state['values']['grouping'])) {
    $groups = explode(PHP_EOL,$form_state['values']['grouping']);
    foreach($groups as $k => $g) {
      $tmp = preg_split('/\s+/',$g);
      if (!empty($tmp[0])) {
        $groups[$k] = array();
        $groups[$k]['line'] = $tmp[0];
        $groups[$k]['group'] = $tmp[1];
        $curr_group = $tmp[1];
        
        // Magic related to only keeping the top 20 groups.
        if (!isset($uniq_groups_with_counts[$curr_group])) {
          $uniq_groups_with_counts[$curr_group] = 0;
          $uniq_groups_indiv[$curr_group] = array();
        }
        $uniq_groups_with_counts[$curr_group]++;
        $uniq_groups_indiv[$curr_group][] = $tmp[0];
        $indiv_mapping[$tmp[0]] = $k;
      }
      else {
        unset($groups[$k]);
      }
    }
  }
  // Otherwise we want to assign all individuals to the same group.
  else {
    $groups = array();
    foreach ($results as $r) {
      $groups[] = array(
        'line' => $r['name'],
        'group' => 'Unknown'
      );  
    }
  }
  
  // We can only support 20 groups due to the number of distiguisable colours available to d3.js
  // If we have more than 20 groups we intend to combine the groups with the least number
  // of individuals until we reach a total of 21.
  $total_count = sizeof($uniq_groups_with_counts);
  
  if ($total_count > 20) {
    drupal_set_message('This chart is limited to 20 groups but you have ' . $total_count . '. We have merged your smallest ' . ($total_count - 20) . ' groups into a single group labelled "Other" to adhere to this restriction.','warning');
  }

  arsort($uniq_groups_with_counts);
  $i = 0;
  $sorted_results = array();
  foreach ($uniq_groups_with_counts as $group_name => $count) {
    $i++;
    foreach ($uniq_groups_indiv[$group_name] as $indiv) {
      // After we've exceeded 20 groups, merge remaining groups into "Other"
      if ($i > 20) {
        $k = $indiv_mapping[$indiv];
        $groups[$k]['group'] = 'Other';
      }

      If (isset($indiv_mapping_results[$indiv])) {
        $k = $indiv_mapping_results[$indiv];
        $sorted_results[] = $results[$k];    
      }
    }
  }
  
  // We would also like to sort the result set to match the group order. This would ensure
  // that the legend matches the group order.
  
  // Now pass both to the javascript chart as settings.
  $settings = array(
    'PCoA' => $sorted_results,
    'groups' => $groups,
  );
  drupal_add_js(array('userCharts' => $settings), 'setting');
  

  // Save the values used to build the chart.
  $form_state['user_charts'] = $settings;
}

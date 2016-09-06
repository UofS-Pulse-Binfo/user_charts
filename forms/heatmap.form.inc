<?php

/**
 * Implements the Form API.
 * Provides a form for users to enter data and generate a heatmap diagram.
 */
function user_charts_heatmap_form($form, &$form_state) {

  // Add the javascript libraries needed for this chart.
  libraries_load('saveSvgAsPng');
  $form['#attached']['js'] = array(
    'http://d3js.org/d3.v3.min.js',
    drupal_get_path('module','user_charts') . '/js/heatmap.chart.js',
    drupal_get_path('module','user_charts') . '/js/saveSVGasPNG.js',
  );

  // Also add the css.
  $form['#attached']['css'] = array(
    drupal_get_path('module','user_charts') . '/css/save_svg.css',
  );

  // DATA ///////////////////
  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #1: Data'),
    '#collapsible' => TRUE,
    '#collapsed' => (isset($form_state['values'])) ? TRUE : FALSE,
  );

  $form['data']['instruct'] = array(
    '#type' => 'item',
    '#markup' => 'Paste a tab delimited copy of your data into the following box. Your data
      should <strong>NOT INCLUDE A HEADER</strong> and should consist of the following 4
        columns:
        <ol>
          <li>Genotype/Germplasm: This should be the name of the germplasm measured as you would like it to appear in the diagram.</li>
          <li>Trait: This should be the name of the trait measured including units.</li>
          <li>Treatment: The name of the treatment or condition the trait was measured under.</li>
          <li>Value: The value measured for that trait, treatment, germplasm combination. This value should have the correct significant figures since it is used to gereate the scale.</li>
        </ol>'
  );

  $form['data']['data'] = array(
    '#title' => 'Full Dataset',
    '#required' => TRUE,
    '#type' => 'textarea',
  );

  // EXAMPLE ////////////////
  $form['data']['example'] = array(
    '#type' => 'fieldset',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#title' => 'Example'
  );

  // Copy for elements for the example.
  $form['data']['example']['example_data'] = $form['data']['data'];

  // Assign the example data to the example fields.
  $form['data']['example']['example_data']['#value'] = user_charts_heatmap_example_data();

  // Make these fields disabled so users don't edit my examples ;-).
  $form['data']['example']['example_data']['#disabled'] = TRUE;

  // Save Example for use later.
  $form_state['example'] = $form['data']['example']['example_data']['#value'];

  // Now give them a button to render it.
  $form['data']['example']['show_example'] = array(
    '#type' => 'submit',
    '#value' => 'Show Example',
    '#attributes' => array('class' => array('example-button')),
    '#submit' => array('user_charts_heatmap_form_submit'),
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
      '#type' => 'markup',
      '#markup' => '<div id="download-svg-link">'
      . 'Save the chart as PNG'
      . '</div>',
    );
  }

  return $form;
}

/**
 * Validate the user data before attempting to draw a chart.
 */
function user_charts_heatmap_form_validate($form, &$form_state) {
  $errors = array();

  // If they pressed the "Show Example" button then use the example data.
  if ($form_state['triggering_element']['#value'] == 'Show Example') {
    $results = $form_state['example'];
    $form_state['show_example'] = TRUE;
  }
  else {
    $results = $form_state['input']['data'];
    $form_state['show_example'] = FALSE;
  }

  // Intitialize a place to store our processed data.
  $form_state['processed'] = array(
    'genotypes' => array(),
    'traits' => array(),
    'treatments' => array(),
    'data' => array(),
  );

  // Validate that the PCoA Results is tab/multi-space delimited
  //   and contains at least 3 columns.
  $results = explode(PHP_EOL, $results);
  $i = 0;
  $stats = array();
  foreach($results as $k => $r) {
    $i++;
    if (!empty($r)) {
      $tmp = preg_split('/(\t|\s\s+)/',$r);

      // CHECK #1: There should be 4 columns.
      if (sizeof($tmp) < 4) {
        $error[] = t('Row %i ("%r") must contain 4 columns. Currently columns are interpreted as: %columns. Make sure your data is separated by a tab or 2+ spaces.',
          array('%i' => $i, '%r' => $r, '%columns' => '"'.implode('","', $tmp).'"'));
      }
      // Do more checks.
      else {

        // Trim the fields so white space doesn't affect the following tests.
        $tmp[0] = trim($tmp[0]);
        $tmp[1] = trim($tmp[1]);
        $tmp[2] = trim($tmp[2]);
        $tmp[3] = trim($tmp[3]);

        // Keep track of some stats so we can do checks later.
        if (isset($stats[ $tmp[1] ][ $tmp[2] ][$tmp[0]])) {
          $stats[ $tmp[1] ][ $tmp[2] ][$tmp[0]]++;
        }
        else {
          $stats[ $tmp[1] ][ $tmp[2] ][$tmp[0]] = 1;
        }

        // Save the data for use in the form submit.
        $form_state['processed']['genotypes'][ $tmp[0] ] = $tmp[0];
        $form_state['processed']['traits'][ $tmp[1] ] = $tmp[1];
        $form_state['processed']['treatments'][ $tmp[2] ] = $tmp[2];
        $form_state['processed']['data'][] = array(
          'Genotype' => $tmp[0],
          'Trait' => $tmp[1],
          'Treatment' => $tmp[2],
          'Value' => $tmp[3],
        );
      }
    }
  }

  // CHECK 1) each trait has all the treatments,
  if ($stats) {
    foreach ($stats as $trait => $treatments) {
      $diff = array_diff($form_state['processed']['treatments'], array_keys($treatments));
      if (!empty($diff)) {
        $error[] = t('All Treatments should be measured for each Trait. You forgot to include the following treatments in %trait: %treatments.',
          array('%trait' => $trait, '%treatments' => implode(',', $diff)));
      }

      // CHECK 2) each treatment has all the genotypes,
      foreach ($treatments as $treatment => $genotypes) {
        $diff = array_diff($form_state['processed']['genotypes'], array_keys($genotypes));
        if (!empty($diff)) {
          $error[] = t('All genotypes/germplasm should be measured for each treatment. You forgot to include the following genotypes in %trait, %treatment: %genotypes.',
            array('%trait' => $trait, '%treatment' => $treatment, '%genotypes' => implode(',', $diff)));
        }

        // CHECK 3) each genotype only has one value.
        $extra = array();
        foreach($genotypes as $genotype => $count) {
          if ($count > 1) {
            $extra[] = $genotype;
          }
        }
        if (!empty($extra)) {
          $error[] = t('You have recorded more than one value for the following genotypes in %trait, %treatment: %genotype',
            array('%trait' => $trait, '%treatment' => $treatment, '%genotype' => implode(',', $extra)));
        }
      }
    }
  }

  if (!empty($error)) {
    form_set_error('data', theme('item_list', array('items' => $error)));
  }

}

/**
 * Final processing of the data before making it available to javascript.
 */
function user_charts_heatmap_form_submit($form, &$form_state) {

  $form_state["rebuild"] = TRUE;

  // Make sure the labels are arrays.
  $traits = array(); $treatments = array(); $genotypes = array();
  foreach ($form_state['processed']['traits'] as $t) $traits[] = $t;
  foreach ($form_state['processed']['treatments'] as $t) $treatments[] = $t;
  foreach ($form_state['processed']['genotypes'] as $g) $genotypes[] = $g;

  // Asign the colours.
  $avail_colors = array(
    array('#edf8e9','#c7e9c0','#a1d99b','#74c476','#31a354','#006d2c'), // Green
    array('#eff3ff','#c6dbef','#9ecae1','#6baed6','#3182bd','#08519c'), // Blue
    array('#f2f0f7','#dadaeb','#bcbddc','#9e9ac8','#756bb1','#54278f'), // Purple
    array('#fee5d9','#fcbba1','#fc9272','#fb6a4a','#de2d26','#a50f15'), // Red
    array('#f7f7f7','#d9d9d9','#bdbdbd','#969696','#636363','#252525'), // Black
    array('#feedde','#fdd0a2','#fdae6b','#fd8d3c','#e6550d','#a63603'), // Orange
  );
  $colors = array();
  $i=0;
  foreach($traits as $trait) {
    if (!isset($avail_colors[$i])) $i=0;
    $colors[$trait] = $avail_colors[$i];
    $i++;
  }

  // Finally compile the settings.
  $settings = array(
    'labels' => array(
      'Traits' => $traits,
      'Treatments' => $treatments,
      'Genotypes' => $genotypes,
    ),
    'colors' => $colors,
    'data' => $form_state['processed']['data'],
  );
  drupal_add_js(array('userCharts' => array('heatmap' => $settings)), 'setting');

}

/**
 * Grab the example data from a file for use in the form.
 */
function user_charts_heatmap_example_data() {
  $file = drupal_get_path('module', 'user_charts') . '/example_data/heatmap.tsv';
  return file_get_contents(drupal_realpath($file));
}

<?php
/**
 * Implements the Form API.
 * Provides a form for users to enter data and generate a beanplot diagram.
 */
function user_charts_beanplot_form($form, &$form_state) {
  // 1. DATA
  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #1 Data'),
    '#collapsible' => TRUE,
    '#collapsed' => isset($form_state['values']) ? TRUE : FALSE,
  );


    // TRAITS
    // Instructions - Trait field.
    $form['data']['instruct_trait'] = array(
      '#type' => 'item',
      '#markup' => '1. Trait should be the name of the trait measured including units. <br />
      <em>e.g. Days till 10% of Plants have Elongated Tendrils (days)</em>',
    );

    // Field - Trait field
    $form['data']['trait'] = array(
      '#title' => 'Trait',
      '#type' => 'textfield',
      '#required' => TRUE,
      '#id' => 'user-chart-trait-field',
      '#attributes' => array(
        'style' => 'width: 98.5%',
      )
    );
    //


    // DATA FIELD
    // Instructions - Data field.
    $form['data']['instruct_data'] = array(
      '#type' => 'item',
      '#markup' => '2. Paste a tab delimeted copy of your data into the
        following box. Your data should <strong>NOT INCLUDE A HEADER</strong>
        and should consists of the following 4 columns:

        <ol>
          <li><em>Location</em>: Lorem ipsum dolor sit amet, consectetur adipiscing elit.</li>
          <li><em>Year</em>: Lorem ipsum dolor sit amet, consectetur adipiscing elit.</li>
          <li><em>Germplasm</em>: Lorem ipsum dolor sit amet, consectetur adipiscing elit.</li>
          <li><em>Mean</em>: Lorem ipsum dolor sit amet, consectetur adispiscing elit.</li>
        </ol>',
    );

    // Field - Data field.
    $form['data']['data'] = array(
      '#title' => 'Full Dataset',
      '#type' => 'textarea',
      '#required' => TRUE,
      '#id' => 'user-chart-data-field',
    );
    //


    // OPTIONAL FIELDS.
    // Summarize datase into a table and highlight germplasm.
    $form['data']['instruct_options'] = array(
      '#type' => 'item',
      '#markup' => '3. Chart options.',
    );

    $form['data']['table'] = array(
      '#type' => 'checkbox',
      '#title' => 'Create a summary table',
    );

    $form['data']['highlight'] = array(
      '#type' => 'checkbox',
      '#title' => 'Highlight Gemplasm',
      '#id' => 'user-charts-hightlight-gy-checkbox',
      '#ajax' => array(
        'event' => 'click',
        'callback' => 'ajax_callback_select_germ',
        'wrapper' => 'ajax-wrapper-select-germ',
        'progress' => array('type' => '', 'message' => ''),
      ),
    );

      $form['data']['ajax_wrapper'] = array(
        '#type' => 'markup',
        '#prefix' => '<div id="ajax-wrapper-select-germ">',
        '#suffix' => '</div>',
      );

      if (isset($form_state['values']['data']) && !empty($form_state['values']['data'])
        && isset($form_state['values']['trait']) && !empty($form_state['values']['trait'])
        && isset($form_state['values']['highlight']) && $form_state['values']['highlight'] == 1) {

        $data = $form_state['values']['data'];
        // Parse dataset and extract all germplasm name.
        $germplasm = user_charts_get_germyear($data);

        if (count($germplasm) > 0) {
          $form['data']['ajax_wrapper']['germplasm'] = array(
            '#type' => 'select',
            '#title' => 'Select Germplasm:',
            '#options' => $germplasm,
          );
        }
        else {
          $form['data']['ajax_wrapper']['no_germ'] = array(
            '#type' => 'item',
            '#markup' => '<div class="error messages">Dataset does not contain Germplasm column.</div>',
          );
        }
      }
    //


    // EXAMPLE DATA.
    $form['data']['example'] = array(
      '#type' => 'fieldset',
      '#title' => 'Example',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

      $form['data']['example']['example_trait'] = array(
        '#type' => 'textfield',
        '#title' => 'Trait:',
        '#disabled' => TRUE,
        '#id' => 'user-chart-example-trait-field',
        '#value' => 'Days till 10% of Plants have Elongated Tendrils (days)',
        '#attributes' => array(
          'style' => 'width: 98.5%',
        ),
      );

      $form_state['example']['example_trait'] = 'Days till 10% of Plants have Elongated Tendrils (days)';


      $form['data']['example']['example_data'] = array(
        '#type' => 'textarea',
        '#title' => 'Full Dataset',
        '#disabled' => TRUE,
        '#value' => user_charts_beanplot_example_data(),
      );

      $form_state['example']['example_data'] = user_charts_beanplot_example_data();


      $form['data']['example']['example_table'] = array(
        '#type' => 'checkbox',
        '#title' => 'Create a summary table',
        '#value' => 1,
        '#disabled' => TRUE,
      );

      $form_state['example']['example_table'] = 1;


      $form['data']['example']['example_highlight'] = array(
        '#type' => 'checkbox',
        '#title' => 'Highlight Gemplasm/Year',
        '#value' => 1,
        '#disabled' => TRUE,
      );

      $form_state['example']['example_highlight'] = 1;


      $form['data']['example']['example_germplasm'] = array(
        '#type' => 'select',
        '#title' => 'Select Germplasm:',
        '#options' => array('GERM:X'),
        '#default_value' => 1,
        '#disabled' => TRUE,
      );

      $form_state['example']['example_germplasm'] = 'GERM:X';


      // Button to render the sample data.
      $form['data']['example']['show_example'] = array(
        '#type' => 'submit',
        '#value' => 'Show Example',
        '#attributes' => array('class' => array('example-button')),
        '#submit' => array('user_charts_beanplot_form_submit'),
        '#limit_validation_errors' => array(array('example')),
      );
    //


  // 2. CHART/VISUALIZATION.
  $form['chart'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #2 Visualization'),
    '#collapsible' => TRUE,
    '#collapsed' => isset($form_state['values']) ? FALSE : TRUE,
  );

    if (isset($form_state['triggering_element']['#value'])
        && $form_state['triggering_element']['#value'] == 'Show Example') {

      // When show example is clicked.
      $form['chart']['msg'] = array(
        '#type' => 'markup',
        '#prefix' => '<div class="messages warning">',
        '#suffix' => '</div><br />',
        '#markup' => 'The following chart is showing EXAMPLE DATA.
          To show your own data, click "Generate" at the bottom of this form.',
      );
    }

    // Canvas.
    $form['chart']['placeholder'] = array(
      '#type' => 'markup',
      '#markup' => '<div id="beanplot-container" style="height: 500px; width: 100%;"></div>',
    );


  // 3. GENERATE BUTTON.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Generate',
  );

    if (isset($form_state['values'])) {
      $form['download'] = array(
        '#type' => 'markup',
        '#markup' => '<div id="download-svg-link">Save the chart as PNG</div>',
      );
    }


  // Style, Javascript and Libraries.
  drupal_add_css(
    '#summary-table {
      width: 90%;
      margin: 0 auto;
    }', 'inline');


  $form['#attached']['css'] = array(
    drupal_get_path('module','user_charts') . '/css/save_svg.css',
  );

  libraries_load('saveSvgAsPng');
  $form['#attached']['js'] = array(
    'http://d3js.org/d3.v3.min.js',
    drupal_get_path('module','user_charts') . '/js/beanplot.chart.js',
    drupal_get_path('module','user_charts') . '/js/saveSVGasPNG.js',
  );


  return $form;
}


/**
 * Function callback: generate options in select germplasm.
 */
function ajax_callback_select_germ($form, $form_state) {
  return $form['data']['ajax_wrapper'];
}


/**
 * Final processing of the data before making it available to javascript.
 */
function user_charts_beanplot_form_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $germplasm = '';

  // Get all form values entered by user.
  if ($form_state['triggering_element']['#value'] == 'Show Example') {
    $example = $form_state['example'];

    $trait          = $example['example_trait'];
    $dataset        = $example['example_data'];
    $with_table     = $example['example_table'];
    $highlight_germ = $example['example_highlight'];

    // When user wants to highlight the germplasm/year combination.
    if ($highlight_germ == 1) {
      $germplasm = trim($example['example_germplasm']);
    }
  }
  else {
    $trait          = $form_state['values']['trait'];
    $dataset        = $form_state['values']['data'];
    $with_table     = $form_state['values']['table'];
    $highlight_germ = $form_state['values']['highlight'];

    // When user wants to highlight the germplasm/year combination.
    if ($highlight_germ == 1) {
      $germplasm = trim($form_state['values']['germplasm']);
    }
  }

  // Given the full dataset, create a summary in the following format
  // location, year, mean, no of germ with this mean, flag if germ was measured
  $each_row = explode("\n", $dataset);
  $arr_summary = array();

  foreach($each_row as $row) {
    if (trim($row)) {
      $tab_values = explode("\t", $row);

      $location = trim($tab_values[0]);
      $year     = trim($tab_values[1]);
      $germ     = trim($tab_values[2]);
      $mean     = trim($tab_values[3]);

      $index = $location . '-' . $year . '-' . $mean;

      // Flag this row if user wants it highlighted in the chart.
      $is_germ = 0;
      if ($highlight_germ == 1) {
        $is_germ = ($germplasm == $germ) ? 1 : 0;
      }

      if (in_array($index, array_keys($arr_summary))) {
        $count = (int)$arr_summary[$index]['no'];

        $arr_summary[$index]['no'] = ($count + 1);

        if ($arr_summary[$index]['germ'] != 1) {
          $arr_summary[$index]['germ'] = $is_germ;
        }
      }
      else {
        $mean = strstr($mean, '.') ? (float)$mean : (int)$mean;

        $arr_summary[$index] = array(
          'location' => $location,
          'year'     => $year,
          'germ'     => $is_germ,
          'mean'     => $mean,
          'no'       => 1
        );
      }
    }
  }

  ksort($arr_summary);

  // This will summarize the data for d3.
  $json_data = array(
    'germplasm'     => $germplasm,
    'phenotype'     => $trait,
    'summary_table' => $with_table,
    'dataset'       => drupal_json_encode(array_values($arr_summary))
  );

  drupal_add_js(array('userCharts' => array('beanplot' => $json_data)), array('type' => 'setting'));
}


/**
 * Validate the user data before attempting to draw a chart.
 */
function user_charts_beanplot_form_validate($form, &$form_state) {
  if ($form_state['triggering_element']['#value'] == 'Generate') {
    $arr_errors = array();

    // Trait field.
    $trait = $form_state['values']['trait'];
    if (strlen($trait) < 5) {
      form_set_error('trait', t('Invalid trait supplied.'));
    }

    // Full dataset
    $dataset = $form_state['values']['data'];
    if (strlen($dataset) < 5) {
      form_set_error('data', t('Invalid dataset supplied.'));
    }
    else {
      // Test for tabs.
      // All line must be 4 columns.
      $m = explode("\n", $dataset);
      $i = 0;
      foreach($m as $l) {

        if (trim($l)) {
          $e = explode("\t", $l);

          if (count($e) < 4) {
            array_push($arr_errors, '#' . ($i + 1));
          }
          else {
            // Test mean value. The last col index = 3.
            $mean = trim($e[3]);

            if (!is_numeric($mean)) {
              // Mean no number.
              array_push($arr_errors, '#' . ($i + 1));
            }
          }
        }

        $i++;

      }

      // Report when there is error.
      if (count($arr_errors) > 0) {
        $line_no = implode(',', $arr_errors);
        $message = 'Line ' . $line_no . ' failed the number of required columns or the
          mean value is not a number.';

        form_set_error('data', $message);
      }
    }
  }
}


/**
 * Grab the example data from a file fo use in the form.
 */
function user_charts_beanplot_example_data() {
  $file = drupal_get_path('module', 'user_charts') . '/example_data/beanplot.tsv';

  return file_get_contents(drupal_realpath($file));
}


/**
 * Function extract the germplasm and year columns to populate
 * select germplasm and select year select boxes.
 */
function user_charts_get_germyear($data) {
  $each_row = explode("\n", $data);
  $arr_options = array();

  foreach($each_row as $row) {
    if (trim($row)) {
      $tab_values = explode("\t", $row);

      if (count($tab_values) >= 4) {
        $germplasm = trim($tab_values[2]);

        if (!in_array($germplasm, $arr_options)) {
          array_push($arr_options, $germplasm);
        }
      }
    }
  }

  sort($arr_options);
  $o = array_combine($arr_options, $arr_options);

  return $o;
}

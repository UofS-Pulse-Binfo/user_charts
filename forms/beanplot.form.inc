<?php
/**
 * Implements the Form API.
 * Provides a form for users to enter data and generate a beanplot diagram.
 */
function user_charts_beanplot_form($form, &$form_state) {
  // Style, Javascript and Libraries.
  libraries_load('saveSvgAsPng');
  $form['#attached']['js'] = array(
    'http://d3js.org/d3.v3.min.js',
    drupal_get_path('module','user_charts') . '/js/beanplot.chart.js',
    drupal_get_path('module','user_charts') . '/js/saveSVGasPNG.js',
  );

  $form['#attached']['css'] = array(
    drupal_get_path('module','user_charts') . '/css/save_svg.css',
  );

  drupal_add_css(
    '#summary-table {
      width: 90%;
      margin: 0 auto;
    }', 'inline');


  // Determine if fieldset should be collapsed.
  $collapsed = FALSE;
  if (isset($form_state['processed']) && count($form_state['processed']) > 0) {
    $collapsed = TRUE;
  }

  // 1. DATA
  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #1 Data'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );


    // TRAITS
    // Field - Trait field
    $form['data']['trait'] = array(
      '#title' => 'Trait',
      '#type' => 'textfield',
      '#required' => TRUE,
      '#id' => 'user-chart-trait-field',
      '#attributes' => array(
        'style' => 'width: 98.5%',
      ),
      '#description' => t('Trait should be the name of the trait measured including units.
        <br /> <em>e.g. Days till 10% of Plants have Elongated Tendrils (days)</em>'),
    );
    //


    // DATA FIELD
    // Instructions - Data field.
    $form['data']['instruct_data'] = array(
      '#type' => 'item',
      '#markup' => 'Paste a tab delimeted copy of your data into the
        following box. Your data should <strong>NOT INCLUDE A HEADER</strong>
        and should consists of the following 4 columns:

        <ol>
          <li><em>Location</em>: The Full name of the location of the plot the data was recorded in (e.g. Sutherland).</li>
          <li><em>Year</em>: The year the data was recorded in (e.g 2017).</li>
          <li><em>Germplasm</em>: The name of the germplasm measured as you would like it to appear in the diagram (e.g. CDC Redberry).</li>
          <li><em>Mean</em>: The mean of the data points recorded for a given germplasm and siteyear. Specifically, you should average the values across replicates.</li>
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


  // 2. CUSTOMIZE DISPLAY
  $form['customize'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #2 Customize Display'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );


    // OPTIONAL FIELDS.
    $form['customize']['table'] = array(
      '#type' => 'checkbox',
      '#title' => 'Create a summary table showing distribution of mean values.',
    );

    $form['customize']['highlight'] = array(
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

      $form['customize']['ajax_wrapper'] = array(
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
          $form['customize']['ajax_wrapper']['germplasm'] = array(
            '#type' => 'select',
            '#title' => 'Select Germplasm:',
            '#options' => $germplasm,
          );
        }
        else {
          $form['customize']['ajax_wrapper']['no_germ'] = array(
            '#type' => 'item',
            '#markup' => '<div class="error messages">Dataset does not contain Germplasm column.</div>',
          );
        }
      }
    //


  // 2. CHART/VISUALIZATION.
  $form['chart'] = array(
    '#type' => 'fieldset',
    '#title' => t('Step #3 Visualization'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

    if (isset($form_state['show_example']) && $form_state['show_example']) {
      // When show example is clicked.
      $form['chart']['msg'] = array(
        '#type' => 'markup',
        '#prefix' => '<div class="messages warning">',
        '#suffix' => '</div><br />',
        '#markup' => 'The following chart is showing EXAMPLE DATA.
          To show your own data, click "Generate" at the bottom of this form.',
      );
    }

    if (isset($form_state['processed'])) {
      // Canvas.
      $form['chart']['placeholder'] = array(
        '#type' => 'markup',
        '#markup' => '<div id="beanplot-container" style="height: 500px; width: 100%;"></div>',
      );

      $form['chart']['#collapsed'] = FALSE;
    }

    if (isset($form_state['processed']['summary_table'])) {
       $table_headers = $form_state['processed']['summary_table']['headers'];
       $tmp_rows = $form_state['processed']['summary_table']['rows'];

       $table_rows = array();

       foreach($tmp_rows as $row) {
         $table_rows[] = array($row['location'], $row['year'], $row['mean'], $row['no']);
       }

       $form['chart']['summary_table'] = array(
         '#type' => 'markup',
         '#markup' => theme('table', array(
           'header' => $table_headers,
           'rows' => $table_rows,
           'sticky' => FALSE,
           'attributes' => array('id' => 'summary-table'))
         ),
       );
    }


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

  return $form;
}


/**
 * Function callback: generate options in select germplasm.
 */
function ajax_callback_select_germ($form, $form_state) {
  return $form['customize']['ajax_wrapper'];
}


/**
 * Validate the user data before attempting to draw a chart.
 */
function user_charts_beanplot_form_validate($form, &$form_state) {
  $form_state['processed'] = array();
  $form_index = '';

  // Default germplasm to not selected.
  $germplasm = '';

  if ($form_state['triggering_element']['#value'] == 'Show Example') {
    // Use show example form.
    $form_index = 'example';

    // Essential form fields.
    $trait = $form_state[$form_index]['example_trait'];
    $data = $form_state[$form_index]['example_data'];

    // Form options:
    $highlight = $form_state[$form_index]['example_highlight'];
    // Get the germplasm to highlight.
    if ($highlight) {
      $germplasm = $form_state[$form_index]['example_germplasm'];
    }

    $table = $form_state[$form_index]['example_table'];

    $form_state['show_example'] = TRUE;
  }
  else {
    // Draw beanplot chart form.
    $form_index = 'input';

    // Essential form fields.
    $trait = $form_state[$form_index]['trait'];
    $data = $form_state[$form_index]['data'];

    // Form options:
    $highlight = $form_state[$form_index]['highlight'];
    // Get the germplasm to highlight.
    if ($highlight) {
      $germplasm = $form_state[$form_index]['germplasm'];
    }

    $table = $form_state[$form_index]['table'];

    $form_state['show_example'] = FALSE;
  }



  // Perform basic test.
  // Form fields are required as specified by #required option, thus
  // when these fields don't get any value, Drupal handles the validation.

  // Proceed to validate the pasted text data.
  if ($data) {
    // Initialize a place to store our processed data.
    $form_state['processed'] = array(
      'trait' => array(),
      'data' => array(),
      'options' => array(),
    );

    // Save trait name and user options.
    $form_state['processed']['trait'] = trim($trait);
    $form_state['processed']['options'] = array(
      'highlight' => $highlight,
      'germplasm' => $germplasm,
      'table' => $table
    );

    // Read each row in the data.
    $rows = explode("\n", $data);
    // Array to hold errors found.
    $arr_errors = array();
    // Line error message.
    $line_error = array(
      'missing_col' => 'Line #%i (%line) : Requires four (4) columns, %columns detected',
      'not_number'  => 'Line #%i (%line) : The mean (4th column) should be a number.',
    );

    $i = 0;
    foreach($rows as $line) {
      if (trim($line)) {
        // Line is not empty.
        // Read each tab, 2 or more spaces.
        $cols = preg_split('/(\t|\s\s+)/', trim($line));

        if (count($cols) < 4) {
          // A line must contain 4 columns.
          $arr_errors[] = t($line_error['missing_col'], array(
            '%i' => $i + 1,   // Line counter.
            '%line' => $line, // Current line.
            '%columns' => count($cols)
          ));
        }
        else {
          // Check the type of the last column, which is the mean value.
          $mean = trim($cols[3]);

          if (!is_numeric($mean)) {
            // Mean is not a number.
            $arr_errors[] = t($line_error['not_number'], array(
              '%i' => $i + 1,
              '%line' => $line
            ));
          }
          else {
            // Line has 4 cols and mean is number. save line.
            $form_state['processed']['data'][] = array(
              'location' => trim($cols[0]),
              'year'     => trim($cols[1]),
              'germ'     => trim($cols[2]),
              'mean'     => trim($cols[3]),
            );
          }
        }
      }

      $i++;
    }

    // Report any errors found.
    if (count($arr_errors) > 0) {
      // Found errors.
      form_set_error('data', theme('item_list', array('items' => $arr_errors)));
    }
  }
}


/**
 * Final processing of the data before making it available to javascript.
 */
function user_charts_beanplot_form_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;

  // Summarize the data.
  $trait = $form_state['processed']['trait'];
  // Data array.
  $data = $form_state['processed']['data'];
  // User options.
  $highlight = $form_state['processed']['options']['highlight'];
  $germplasm = $form_state['processed']['options']['germplasm'];
  $table = $form_state['processed']['options']['table'];

  $arr_summary = array();

  foreach($data as $item) {
    list($location, $year, $germ, $mean) = array_values($item);

    $index = $location . '-' . $year . '-' . $mean;

    // Flag this line if user wants it highlighted in the chart.
    $is_germ = 0;

    if ($highlight == 1) {
      $is_germ = ($germplasm == $germ) ? 1 : 0;
    }

    if (in_array($index, array_keys($arr_summary))) {
      // If index is found, increment the number of times it appeard in the dataset.
      $count = (int)$arr_summary[$index]['no'];

      $arr_summary[$index]['no'] = ($count + 1);

      if ($arr_summary[$index]['germ'] != 1) {
        $arr_summary[$index]['germ'] = $is_germ;
      }
    }
    else {
      $mean = strstr($mean, '.') ? (float) $mean : (int)$mean;

      $arr_summary[$index] = array(
        'location' => $location,
        'year'     => $year,
        'germ'     => $is_germ,
        'mean'     => $mean,
        'no'       => 1
      );
    }
  }

  // Sort array.
  ksort($arr_summary);

  // Make the summary data available to theme table in case user
  // wants a summary table of mean values.
  if ($table == 1) {
    $form_state['processed']['summary_table'] = array(
      'headers' => array('Location', 'Year', 'Mean', 'Germplasm'),
      'rows' => $arr_summary,
    );
  }

  // Finally, compile the settings.
  $settings = array(
    'trait' => $trait,
    'germplasm' => $form_state['processed']['options']['germplasm'],
    'data_JSON' => drupal_json_encode(array_values($arr_summary)),
  );

  drupal_add_js(array('userCharts' => array('beanplot' => $settings)), 'setting');
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

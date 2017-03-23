(function($) {
  Drupal.behaviors.userchartsBeanplot = {
    attach: function (context, settings) {
    //
      var fldTrait = $('#user-chart-trait-field');
      var fldData  = $('#user-chart-data-field');

      // Reset the form when data fieldset is clicked.
      $('#edit-data .fieldset-title').click(function() {
        if ($('svg').length > 0) {
          $('g, svg, #download-svg-link, #beanplot-container h3, #beanplot-container').remove();
          $('#edit-chart .fieldset-title').click();

          if ($('#summary-table')) {
            $('#summary-table').remove();
          }

          if ($('.error').length > 0) {
            $('.warning').remove();
          }

          $('#edit-submit').show();
        }
      });

      if (Drupal.settings.hasOwnProperty('userCharts')) {
      //
        // Add event listener to when window is resized.
        d3.select(window).on('resize', renderBP);

        // Prepare dataset.
        var stringData = Drupal.settings.userCharts.beanplot;

        // Germplasm.
        var germplasm = stringData['germplasm'];
        // Trait name.
        var phenotype = stringData['phenotype'];
        // Summary table.
        var summaryTable = stringData['summary_table'];
        // Convert the string data to object.
        var dataset = JSON.parse(stringData['dataset']);

        // Arrange dataset by year and location.
        var location_year = d3.nest()
          .key(function(d) { return d.year + ':' + d.location; })
          .sortKeys(d3.ascending)
          .entries(dataset);

        // Mean, max, min and no values.
        var allMeanValues = dataset.map(function(d) { return d.mean; });
        var decimalVal = allMeanValues.filter(function(a) {
          return (a % 1 > 0);
        });

        var lyCount = location_year.length,
          // Max mean value.
          maxMean = d3.max(allMeanValues),
          // Min mean value.
          minMean = d3.min(allMeanValues),
          // Max data count.
          maxNo   = d3.max(dataset.map(function(d) { return d.no;   }));

          maxMean = Math.floor(maxMean);
          minMean = Math.floor(minMean);


        // Let's chart!
        // On initial load, get window height and width of the container.
        var svg, x, xAxis, y, yAxis, zAxis, height, width;
        getBPContainerDimension();

        // Canvas.
        svg = d3.select('#beanplot-container')
          .append('svg')
            .attr('id', 'beanplot-svg-container');

        if ($('#beanplot-container')) {
          $('#edit-submit').hide();
        }

        // When user wants a summary table.
        if (summaryTable == 1) {
          var tblHeader = ['Location', 'Year', 'Mean', 'Germplasm'];

          var div = d3.select('#edit-chart')
            .append('div')
              .attr('id', 'summary-table');

          div.append('h3')
            .text('Distribution of mean values summary table');

          var table = div
            .append('table');

          var tableHead = table
            .append('thead')
            .append('tr');

          tableHead
            .selectAll('thead tr')
            .data(tblHeader)
            .enter()
            .append('th')
            .text(function(d) {
              return d;
            });

          var tableRow = table
            .append('tbody');

          var rows = tableRow
            .selectAll('tbody tr')
            .data(dataset)
            .enter()
            .append('tr')
            .attr('class', function(d, i) {
              return (i % 2) ? 'even' : 'odd';
            });

          rows.append('td').html(function(m) { return m.location; });
          rows.append('td').html(function(m) { return m.year;     });
          rows.append('td').html(function(m) { return m.mean;     });
          rows.append('td').html(function(m) { return m.no;       });
        }

        // Chart margins and dimensions.
        // Height or width of g container of both axes.
        // Adjust to zoom in and out.
        var axesHW = 70;

        // Main chart g container.
        var bpMargin      = {};
          bpMargin.top    = 5;
          bpMargin.right  = 10;
          // Double the bottom margin in to account for the chart title.
          bpMargin.bottom = axesHW * 2;
          bpMargin.left   = axesHW;
          bpMargin.gutter = 5;

        // Define Gradient.
        var bpGradient = svg.append('defs')
          .append('linearGradient')
          .attr('id', 'bp-gradient')
          .attr('x1', '0%')
          .attr('x2', '100%')
          .attr('x2', '0%')
          .attr('y2', '100%')
          .attr('spreadMethod', 'pad');

        bpGradient.append('stop')
          .attr('offset', '0%')
          .attr('stop-color', '#EAEAEA')
          .attr('stop-opacity', 1);

        bpGradient.append('stop')
          .attr('offset', '100%')
          .attr('stop-color', '#C3C3C3')
          .attr('stop-opacity', 1);

        var bpGradientGerm = svg.append('defs')
          .append('linearGradient')
          .attr('id', 'bp-gradient-germ')
          .attr('x1', '0%')
          .attr('x2', '80%')
          .attr('x2', '0%')
          .attr('y2', '100%')
          .attr('spreadMethod', 'pad');

        bpGradientGerm.append('stop')
          .attr('offset', '0%')
          .attr('stop-color', '#AFD0A1')
          .attr('stop-opacity', 1);

        bpGradientGerm.append('stop')
          .attr('offset', '100%')
          .attr('stop-color', '#9AC585')
          .attr('stop-opacity', 1);

        // Tool tip.
        var infoBox = d3.select('body')
          .append('div')
          .attr('class', 'tool-tip')
          .style({
            'background'     : 'lightgreen',
            'border'         : '1px solid #000000',
            'border-radius'  : '4px',
            'font'           : '12px sans-serif',
            'padding'        : '10px',
            'position'       : 'absolute',
            'pointer-events' : 'none',
            'text-align'     : 'center',
            'opacity'        : 0,
            'top'            : '-30px'
          });

        // Y
        var yAxisWrapper = svg.append('g')
          .attr('id', 'bp-g-yaxiswrapper')
          .attr('class', 'bp-g-axis')

        var yLabel = 'Mean Observed Values per Site Year';
        yAxisWrapper.append('g')
          .attr('id', 'bp-y-text-label')
          .append('text')
            .text(yLabel);

        // Scale.
        var newMaxMean = top(maxMean),
            newMinMean = bottom(minMean);

        y = d3.scale.linear()
          .domain([newMinMean, newMaxMean])
          .range([(height - bpMargin.top - bpMargin.bottom - bpMargin.gutter), 0])
          .nice();

        var ticks     = y.ticks(),
            ticksMax  = d3.max(ticks),
            ticksPair = d3.pairs(ticks),
            ticksDiff = ticksPair[0][1] - ticksPair[0][0],
            ticksAll  = []

        // Coerce d3 to add more tick marks.
        for(var f = newMinMean; f <= ticksMax; f++) {
          ticksAll.push(f);
        }

        // Add the mean values with decimal places.
        ticksAll = ticksAll.concat(decimalVal);
        ticksNo = ticksAll.length;

        // Sort tick values.
        ticksAll.sort(function(a, b) { return a-b; });

        if (ticksAll[0] > 0) {
          var breakLineHeight = 29;

          // Break line when y axis scale does not strat at 0.
          var breakLine = yAxisWrapper
            .append('g')
              .attr('id', 'break-line')
              .append('polyline')
                .attr('fill', 'none')
                .attr('stroke', '#000000')
                .attr('shape-rendering', 'crispEdges');

          // Break line at 0.
          var isZero =  yAxisWrapper
              .append('g')
              .style('font', '10px sans-serif')
                .append('text')
                .text('0');
        }

        yAxis = d3.svg.axis()
          .scale(y)
          .orient('left')
          .ticks(ticksNo)
          .tickValues(ticksAll)
          .tickFormat(d3.format('d'));

        var yMeanValues = yAxisWrapper
          .append('g')
            .attr('id', 'bp-y-scale')
            .call(yAxis);

        // Create a sub-tick marks by not displaying the value.
        d3.selectAll('#bp-y-scale text')
          .attr('x', -15)
          .text(function(d) {
            return (d % ticksDiff == 0) ? d : null;
          });

        // Indent such tick marks.
        d3.selectAll('#bp-y-scale line')
          .attr('x2', function(d) {
            return (d % ticksDiff == 0) ? -12 : -6;
          });

        // X
        var xAxisWrapper = svg.append('g')
          .attr('id', 'bp-g-xaxiswrapper')
          .attr('class', 'bp-g-axis');

        // Label.
        var xLabel = 'The Distribution of Mean ' + phenotype + ' per Site Year';
        xAxisWrapper.append('g')
          .attr('id', 'bp-x-text-label')
          .style('font-size', '1.2em')
          .append('text')
            .attr('class', 'text-axes-label')
            .text(xLabel);

        x = d3.scale.ordinal()
          .domain(location_year.map(function(d) {
            return d.key;
          }));

        xAxis = d3.svg.axis()
          .orient('bottom');

        var xLocationYear = xAxisWrapper.append('g')
          .attr('id', 'bp-x-scale')
          .attr('class', 'bp-axis');

        // Z
        var z = d3.scale.linear();

        // Beanplot elements.
        // g container where all chart elements will be rendered.
        var chartWrapper = svg
          .append('g')
            .attr('id', 'bp-g-chartwrapper');

        // Create g container for each bean/plot.
        var bpWrapper = chartWrapper
          .selectAll('g')
          .data(location_year)
          .enter()
          .append('g')
            .attr('id', function(d, i) {
              return 'bp-location-year-' + i;
            })
            .attr('class', 'bp-g-location-year');

        var rect = bpWrapper.selectAll('rect')
          .data(function(d) { return d.values; })
          .enter()
          .append('g')
            .on('mousemove', function(d) {
              d3.select(this).style('opacity', 0.5);

              infoBox.transition().style('opacity', 1);
              infoBox
                .html('Mean: ' + d.mean + ' (' + d.no + ' Germplasm)')
                .style('left', (d3.event.pageX + 10) + 'px')
                .style('top', (d3.event.pageY) + 'px');
            })
            .on('mouseout', function(d) {
              d3.select(this).style('opacity', 1);
              infoBox.transition().style('opacity', 0);
            })
          .append('rect')
            .attr('fill', function(d) {
              return (d.germ == 1) ? 'url(#bp-gradient-germ)' : 'url(#bp-gradient)';
            })
            .attr('stroke', function(d) {
              return (d.germ == 1) ? '#72AB4D' : '#A5A5A5';
            });


        // Legend when germplasm is to be highlighted.
        if (germplasm != '') {
          var legend = svg
            .append('g')
              .attr('id', 'bp-legend');

          legend
            .append('rect')
            .attr('fill', 'url(#bp-gradient-germ)')
            .attr('stroke', '#72AB4D')
            .attr('height', 10)
            .attr('width', 20);

          legend
            .append('text')
            .text(germplasm + ' measured')
            .attr('text-anchor', 'end')
            .attr('x', -7)
            .attr('y', 9)
            .style({
              'font-size'   : '11px',
              'font-family' : 'sans-serif',
              'font-weight' : 300,
              'stroke'      : 'none',
              'text-anchor' : 'right',
              'width'       : '120px'
            });
        }


        // Render the chart elements.
        renderBP(0);


        // Style line and text.
        // NOT: External style sheet will not apply when saving
        // this chart to PNG.
        d3.selectAll('#bp-y-scale text')
          .style('font', '10px sans-serif');

        d3.selectAll('line, path')
          .style({
            'fill'            : 'none',
            'stroke'          : '#000000',
            'shape-rendering' : 'crispEdges'
          });

        d3.selectAll('rect')
          .style({
            'shape-rendering' : 'crispEdges',
            'stroke-width'    : 1,
          });
      }


      $(document)
      .ajaxComplete(function() {
        $('#edit-submit').show();
        if (!$('#edit-chart').hasClass('collapsed')) {
          $('#edit-chart .fieldset-title').click();
        }
      });

      if ($('.error').length > 0) {
        $('#edit-chart .fieldset-title').click();
        $('#download-svg-link').hide();
      }

      fldTrait.focus();


      // Position the chart elements to the right coordinates based
      // height and width returned.
      function renderBP(u = 1) {
        // Get the new dimension.
        if (u == 1) {
          getBPContainerDimension();
        }

        var bp = {};
          bp.chartAreaWidth  = width  - bpMargin.left - bpMargin.right  - bpMargin.gutter;
          bp.chartAreaHeight = height - bpMargin.top  - bpMargin.bottom - bpMargin.gutter;
          bp.chartEachly     = Math.round(bp.chartAreaWidth/lyCount);
          bp.chartBarHeight  = bp.chartAreaHeight/ticksAll.length;

        // Apply the new dimension to chart elements.

        // Canvas.
        svg
          .attr('height', height)
          .attr('width', width);

        // Y
        yAxisWrapper
          .attr('transform', 'translate(0, ' + bpMargin.top + ')');

        // label - 20px text height of the y label.
        d3.select('#bp-y-text-label text')
          .attr('transform', function() {
            return 'translate(' + (bpMargin.gutter + 20) + ',' + Math.round((height + bpMargin.bottom)/2) +') rotate(-90)';
          });

        // Scale - Mean values.
        yMeanValues
          .attr('transform', 'translate(' + bpMargin.left + ',' + bpMargin.top + ')');

        // Add the break line.
        if (breakLine && isZero) {
          breakLineHeight = 29;
          breakLine
            .attr('points', function() {
              var cLeft = bpMargin.left;
              var cBottom = bp.chartAreaHeight + (bpMargin.gutter * 2) + 4;

              return cLeft + ',' + (cBottom - 10) + ' ' + cLeft + ',' + cBottom + ' ' + (cLeft + 10) + ',' + cBottom + ' ' + (cLeft - 10) + ',' + (cBottom + 10) + ' ' + cLeft + ',' + (cBottom + 10) + ' ' + cLeft + ',' + (cBottom + 20) + ' ' + (cLeft - 13) + ',' + (cBottom + 20);
            });

          isZero
            .attr('transform', function() {
              return 'translate('+ (bpMargin.left - 22) + ',' + (bp.chartAreaHeight + 38) +')';
            });
        }
        else {
          breakLineHeight = 0;
        }

        // X
        // label - 20px text height of the y label.
        d3.select('#bp-x-text-label')
          .attr('text-anchor', 'middle')
          .attr('transform', function() {
            return 'translate(' + (Math.round(width/2) - bpMargin.left) + ',' + (bpMargin.bottom - 50) + ')';
          });

        // Scale Location and Year.
        xAxisWrapper
          .attr('transform', 'translate(' + bpMargin.left + ',' + (height - bpMargin.bottom + breakLineHeight) + ')');

        x
          .rangeRoundBands([0, bp.chartAreaWidth]);
          xAxis.scale(x);

        xLocationYear
          .call(xAxis)
          .selectAll('#bp-x-scale text')
          .call(wrapWords, x.rangeBand());

        // Z
        z
          .domain([0, maxNo])
          .range([0, bp.chartEachly - bpMargin.gutter - 60]);

        // Beanplot chart.
        chartWrapper
          .attr('height', bp.chartAreaHeight)
          .attr('width', bp.chartAreaWidth)
          .attr('transform', 'translate(' + (bpMargin.left + bpMargin.gutter) + ',' + bpMargin.top + ')');

        d3.selectAll('.bp-g-location-year')
          .attr('width', bp.chartEachly - 60)
          .attr('transform', function(d, i) {
            var x = bp.chartEachly * i;

            return 'translate(' + x + ',' + bpMargin.top + ')';
          });

        rect
          .attr('height', bp.chartBarHeight)
          .style('opacity', 0)
          .attr('width', function(d) {
            return Math.round(z(d.no));
          })
          .attr('x', function(d) {
             var w = d3.select(this).attr('width');
                 w = parseInt(w);

             var wHalf = w/2,
                 ww = bp.chartEachly,
                 wwHalf = ww/2;

             return (wwHalf - wHalf) - bpMargin.gutter;
          })
          .attr('y', function(d) {
            return posBar(d.mean, bp.chartBarHeight);
          })
          .transition()
          .duration(function() {
            return randomNumber();
          })
          .ease('back')
          .style('opacity', 1);


          if (legend) {
            legend
              .attr('transform', 'translate(' + ((width - bpMargin.right) - 20) + ',' + bpMargin.top + ')');
          }
      }



      // HELPER FUNCTIONS:

      // Position the bars into the right y (mean) scale.
      function posBar(mean, h) {
        var index, r;

        //mean = (mean < 0) ? Math.floor(mean) : Math.ceil(mean);
        r = (mean % 1);

        // Find the ticks that corresponds to the mean value.
        // Then get the y coordinate value and use that to plot bar.
        ticksAll.forEach(function(d, i) {
          if (d == mean) {
            index = i;
          }
        });

        var s;
        var g = d3.select('#bp-y-scale').selectAll('.tick')
          .filter(function(d, i) {
            if (i == index) {
              s = d3.select(this).attr('transform');
            }
          });

        var t = d3.transform(s),
        y = Math.round(t.translate[1]);

        return y - Math.round(h/2);
      }


      // Debugging function. Echo the contents of d.
      function echo(d) {
        alert(JSON.stringify(d));
        //console.log(JSON.stringify(d));
        //console.log(d);
      }

      // Get the height and width of the chart container
      // and use it to estimate the amount of area chartable.
      function getBPContainerDimension() {
        // Reference chart container.
        var container = d3.select('#beanplot-container');

        // Get height and width.
        var containerHeight = container.style('height');
        var containerWidth  = container.style('width');

        // Set the the global var accordingly.
        height = parseInt(containerHeight, 10);
        width = parseInt(containerWidth, 10);
      }
      //


      // Function stack text on top of each other.
      // e.g.   2016    (year)
      //      Saskatoon (location)
      function wrapWords(text, width) {
        text.each(function() {
          // Reference text.
          var text  = d3.select(this);
          // Read the words in the text.
          var words = text.text().split(/:/);
          // Clear the text so no duplicate label shown.
          text.text(null);

          var word,
              line = [],
              lineNumber = 0,
              lineHeight = 1 // ems
              y = text.attr('y') - bpMargin.gutter,
              dy = parseFloat(text.attr('dy'));

          while (word = words.pop()) {
            text.append('tspan')
              .attr('class', 'bp-tspan')
              .attr('x', 0)
              .attr('y', y)
              .attr('dy', ++lineNumber * lineHeight + dy + 'em')
              .text(word);
          }
        });
      }


      // Function increase the max mean value.
      function top(v) {
        var newMean;

        // Exclude decimal values.
        v = (v > 0) ? Math.floor(v) : Math.ceil(v);

        var mod     = (v % 10);
        var newMean = v + mod + 10;

        return newMean;
      }


      // Function decrease the min value.
      function bottom(v) {
        var newMean;

        // Exclude decimal values.
        v = (v >= 0) ? Math.floor(v) : Math.ceil(v);

        var mod     = (v % 10);
        var newMean = v - mod - 10;

        if (newMean <= 0) {
          newMean = 0;
        }

        return newMean;
      }


      // Generate random numbers.
      function randomNumber() {
        var min = 1;
        var max = 10;

        return (Math.random() * (max - min) + min ) * 100;
      }
    //
    }
  };
})(jQuery);

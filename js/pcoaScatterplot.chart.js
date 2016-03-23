
Drupal.behaviors.userChartsPCoA = {
  attach: function (context, settings) {
  
    if (Drupal.settings.hasOwnProperty('userCharts')) {
      var data = Drupal.settings.userCharts.PCoA;
      var groupData = Drupal.settings.userCharts.groups;

      // Define the sizes and margins for our canvas.
      var margin = {top: 20, right: 150, bottom: 20, left: 20},
        width = 900 - margin.left - margin.right,
        height = 500 - margin.top - margin.bottom;

      // Cast my values as numbers and determine ranges.
      var minmax = {p1: {min:0, max:0}, p2: {min:0, max:0}}
      data.forEach(function(d) {
        d.p1 = +d.p1;
        d.p2 = +d.p2;
        minmax.p1.min = Math.min(d.p1, minmax.p1.min);
        minmax.p1.max = Math.max(d.p1, minmax.p1.max);
        minmax.p2.min = Math.min(d.p2, minmax.p2.min);
        minmax.p2.max = Math.max(d.p2, minmax.p2.max);
      });

      // Set-up my x scale.
      var x = d3.scale.linear()
        .range([0, width])
        .domain([Math.floor(minmax.p1.min), Math.ceil(minmax.p1.max)]);

      // Set-up my y scale.
      var y = d3.scale.linear()
        .range([height, 0])
        .domain([Math.floor(minmax.p2.min), Math.ceil(minmax.p2.max)]);

      // Create my x-axis using my scale.
      var xAxis = d3.svg.axis()
        .scale(x)
        .orient("bottom");

      // Create my y-axis using my scale.
      var yAxis = d3.svg.axis()
        .scale(y)
        .orient("left");

      // Set-up my colours/groups.
      var color = d3.scale.category20();
      var groups = {};
      groupData.forEach(function(d) {
        groups[d.line] = d.group;
      });

      // Create my tooltip creating function.
      var tip = d3.tip()
        .attr('class', 'd3-tip')
        .offset([-10, 0])
        .html(function(d) {
          return "<strong>" + d.name + "</strong> (" + groups[d.name] + ")";
        });

      // Actually create my canvas.
      var svg = d3.select("#chart").append("svg")
        .attr('id', 'PCoA-chart')
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
      .append("g")
        .attr("transform", "translate(" + margin.left + "," + margin.top + ")");
  
      // Initialize my tooltip.
      svg.call(tip);

      // Draw my x-axis.
      svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + y(0) + ")")
        .call(xAxis)
      .append("text")
        .attr("class", "label")
        .attr("x", width)
        .attr("y", -6)
        .style("text-anchor", "end")
        .text("Coord. 1");

      // Draw my y-axis.
      svg.append("g")
        .attr("class", "y axis")
        .attr("transform", "translate(" + x(0) + ",0)")
        .call(yAxis)
      .append("text")
        .attr("class", "label")
        .attr("transform", "rotate(-90)")
        .attr("y", 6)
        .attr("dy", ".71em")
        .style("text-anchor", "end")
        .text("Coord. 2");

      // Create all the data points :-D.
      svg.selectAll(".dot")
        .data(data)
      .enter().append("circle")
        .attr("class", function(d) { return "dot " + groups[d.name].replace(/\s+/g, '-').toLowerCase(); })
        .attr("r", 3.5)
        .attr("cx", function(d) { return x(d.p1); })
        .attr("cy", function(d) { return y(d.p2); })
        .style("stroke", function(d) { return color(groups[d.name]); })
        .style("fill", function(d) { return color(groups[d.name]); })
        .on('mouseover', tip.show)
        .on('mouseout', tip.hide);

      // Create the container for the legend if it doesn't already exist.
      var legend = svg.selectAll(".legend")
        .data(color.domain())
      .enter().append("g")
        .attr("class", "legend")
        .attr("transform", function(d, i) { return "translate(" + (margin.right - 20) + "," + i * 20 + ")"; });

      // Draw the coloured rectangles for the legend.
      legend.append("rect")
        .attr("x", width - 18)
        .attr("class", function(d) { return d.replace(/\s+/g, '-').toLowerCase(); })
        .attr("width", 18)
        .attr("height", 18)
        .style("fill", color);

      // Draw the labels for the legend.
      legend.append("text")
        .attr("x", width - 24)
        .attr("y", 9)
        .attr("dy", ".35em")
        .style("text-anchor", "end")
        .text(function(d) { return d; });
        
      // Change the "Other" group to black.
      svg.selectAll(".other")
        .style("stroke", "black")
        .style("fill", "black");
        
    }
}};

// Add in the save as png link/download.
// uses external library.
(function ($) {
  Drupal.behaviors.myModuleBehavior = {
    attach: function (context, settings) {

      $('#download-svg-link').click(function() { 
        saveSvgAsPng(document.getElementById("PCoA-chart"), "PCoA_chart.png", {scale: 2, backgroundColor:'#fff'});
      });

    }
  };
})(jQuery);

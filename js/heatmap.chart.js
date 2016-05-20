
Drupal.behaviors.userChartsHeatmap = {
  attach: function (context, settings) {

    var fontFamily = 'Georgia, "Times New Roman", Times, serif',
      fontSize = '13px';

    if (Drupal.settings.hasOwnProperty('userCharts')) {

      numTraits = Drupal.settings.userCharts.heatmap.labels.Traits.length;
      numTreatments = Drupal.settings.userCharts.heatmap.labels.Treatments.length;
      numGenotypes = Drupal.settings.userCharts.heatmap.labels.Genotypes.length;

      // Determine the length fo the longest label for treatment
      // in order to determine what our left axis width should be.
      var maxTreatmentLabelWidth = 0;
      Drupal.settings.userCharts.heatmap.labels.Treatments.forEach(function(d) {
        curLength = getLabelWidth(d);
        console.log(d+'='+curLength);
        if (curLength > maxTreatmentLabelWidth) maxTreatmentLabelWidth = curLength;
      });

      var margin = { top: 20, right: 20, bottom: 20, left: 20 },
        leftAxisWidth = maxTreatmentLabelWidth + 75,
        betweenTraitBuffer = 50,
        gridSize = 40,
        width = leftAxisWidth + (numGenotypes * gridSize) + 50,
        height = numTraits * ((numTreatments * gridSize) + betweenTraitBuffer) + leftAxisWidth;

      // When reading from tsv, numbers are read in as strings.
      // First, we will make them numbers ;-).
      Drupal.settings.userCharts.heatmap.data.forEach(function(d) {
        d.formattedValue = d.Value;
        d.Value = parseFloat(d.Value);
      });

      // Determine the min and max for each trait.
      var min = {}, max = {}, formattedMin = {}, formattedMax = {};
      Drupal.settings.userCharts.heatmap.labels.Traits.forEach(function(d) {
        min[d] = 9999;
        max[d] = 0;
        formattedMin[d] = 9999;
        formattedMax[d] = 0;
      });
      Drupal.settings.userCharts.heatmap.data.forEach(function(d) {
        if (d.Value < min[d.Trait]) { min[d.Trait] = d.Value;  formattedMin[d.Trait] = d.formattedValue; }
        if (d.Value > max[d.Trait]) { max[d.Trait] = d.Value;  formattedMax[d.Trait] = d.formattedValue; }
      });

      // Returns one of the colours above based on bucket the value falls into
      // and the treament it is in.
      var getShade = function(trait, treatment, value) {

        // Determine the range from smallest to largest.
        fullRange = max[trait] - min[trait];
        // and how long each range is.
        eachRange = fullRange / 6;

        // Determine how much our value is above the minimum.
        currValue = value - min[trait];

        // Determine the range that our value falls into.
        bucket = Math.ceil(currValue / eachRange);
        if (bucket == 0) { bucket = 1; }

        // Now pick a colour based on the treatment and bucket.
        return Drupal.settings.userCharts.heatmap.colors[trait][bucket-1];
      };

      var svg = d3.select("#chart").append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .attr("id", "chart-svg")
        .append("g")
        .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

      // Trait Labels.
      svg.selectAll(".label.trait")
        .data(Drupal.settings.userCharts.heatmap.labels.Traits)
        .enter().append("text")
          .text(function (d) { return d; })
          .attr("x", 0)
          .attr("y", function (d, i) {
            var yi = i * numTreatments;
            var ypadding = (i+1) * betweenTraitBuffer;
            return yi * gridSize + ypadding -5;
          })
          .style("text-anchor", "beginning")
          .style("font-weight", "bolder")
          .style("text-decoration", "underline");

      // Scale.
      // There needs to be scale per trait.
      scaleSize = (gridSize * 1.5) / 6;
      scaleOffset = (gridSize * 1.5) / 2;
      betweenScaleBuffer = betweenTraitBuffer + (gridSize * numTreatments);
      Drupal.settings.userCharts.heatmap.labels.Traits.forEach(function(trait, traitI) {
        svg.selectAll(".scale")
          .data(Drupal.settings.userCharts.heatmap.colors[trait])
          .enter().append("rect")
            .attr("width", scaleSize + 10)
            .attr("height", scaleSize)
            .attr("x","10")
            .attr("y", function(d,i) {
              var yi = traitI * numTreatments;
              traitTop = (yi * gridSize) + ((traitI + 1) * betweenTraitBuffer);
              return traitTop + scaleOffset + (i * scaleSize);
            })
            .style("fill", function(d) { return d; });
        svg.selectAll(".label.scale")
          .data([ formattedMin[trait], formattedMax[trait] ])
            .enter().append("text")
            .text(function (d) { return d; })
            .attr("x", "10")
            .attr("y", function(d,i) {
              var yi = Number(traitI) * numTreatments;
              traitTop = (yi * gridSize) + ((traitI + 1) * betweenTraitBuffer);
              if (i=="0") { offset = 27; } else { offset = 110; }
              return traitTop + offset;
            })
            .style("font-size","12px");
      });

      // Treatment Labels.
      // These labels need to be created for each Trait.
      Drupal.settings.userCharts.heatmap.labels.Traits.forEach(function(d, traitI) {
        svg.selectAll(".label.treatment")
          .data(Drupal.settings.userCharts.heatmap.labels.Treatments)
          .enter().append("text")
            .text(function (d) { return d; })
            .attr("x", leftAxisWidth-10)
            .attr("y", function (d, i) {
              var yi = Number(traitI) * numTreatments + i + 1;
              var ypadding = (traitI + 1) * betweenTraitBuffer - (gridSize / 4);
              return yi * gridSize + ypadding -5;
            })
            .style("text-anchor", "end");
      });

      // Genotypes/Germplasm Labels.
      xaxis = svg.append("g")
        .attr("class", "x-axis")
        .attr("transform", "translate(0," + (((numTraits * numTreatments-1) * gridSize) + ((numTraits+1) * betweenTraitBuffer) + 10) + ")");
      xaxis.selectAll("text")
        .data(Drupal.settings.userCharts.heatmap.labels.Genotypes)
        .enter().append("text")
          .text(function (d) { return d; })
          .attr("x", function(d, i) {
            return Number(i) * gridSize + leftAxisWidth + (gridSize/2);
          })
          .attr("y", 0)
          .style("text-anchor", "end")
          //.attr("dx", "-.8em")
          //.attr("dy", ".15em")
          .attr("transform", function(d,i) {
            y = 0;
            x = Number(i) * gridSize + leftAxisWidth + (gridSize/2);
            return "rotate(-65,"+x+","+y+")"
          });


      // Add the cells/boxes.
      svg.selectAll(".cell")
        .data(Drupal.settings.userCharts.heatmap.data)
        .enter().append("rect")
          .attr("x", function(d) {
            genotypeI = Drupal.settings.userCharts.heatmap.labels.Genotypes.indexOf(d.Genotype);
            return genotypeI * gridSize + leftAxisWidth;
          })
          .attr("y", function(d) {
            traitI = Drupal.settings.userCharts.heatmap.labels.Traits.indexOf(d.Trait) + 1;
            treatmentI = Drupal.settings.userCharts.heatmap.labels.Treatments.indexOf(d.Treatment) + 1;
            yi = ((traitI-1) * numTreatments) + (treatmentI);
            ypadding = betweenTraitBuffer * traitI;
            return (yi-1) * gridSize + ypadding;
          })
          .attr("class", function(d) { return d.Genotype+'-'+d.Trait+'-'+d.Treatment; })
          .attr("width", gridSize-1)
          .attr("height", gridSize-1)
          .attr("value", function(d) { return d.Value; })
          .style("fill", function(d) { return getShade(d.Trait, d.Treatment, d.Value);});
  }

  function getLabelWidth(str) {
    var canvas = document.createElement('canvas');
    var ctx = canvas.getContext("2d");
    ctx.font = fontSize + " " + fontFamily;
    var width = ctx.measureText(str).width;
    return width;
  }
}};

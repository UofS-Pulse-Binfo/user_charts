// Add in the save as png link/download.
// uses external library.
(function ($) {
  Drupal.behaviors.saveSvgAsPng = {
    attach: function (context, settings) {

      $('#download-svg-link').click(function() {
        saveSvgAsPng(document.getElementById("chart-svg"), "chart.png", {scale: 2, backgroundColor:'#fff'});
      });

    }
  };
})(jQuery);

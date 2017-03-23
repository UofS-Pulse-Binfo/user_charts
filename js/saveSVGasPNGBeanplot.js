// Add in the save as png link/download.
// uses external library.
(function ($) {
  Drupal.behaviors.saveSvgAsPngBeanplot = {
    attach: function (context, settings) {

      $('#download-svg-link').click(function() {
        saveSvgAsPng(document.getElementById("beanplot-svg-container"), "chart.png", {scale: 2, backgroundColor:'#fff'});
      });

    }
  };
})(jQuery);

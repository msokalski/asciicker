var performance_fake = 0;
var performance = {now:function() { return performance_fake++; }};

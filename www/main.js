

function show_style(){
    var twitter_style = document.getElementsByClassName('twitter-timeline')
    console.log(twitter_style)
}

function display_twitter_embed(){
    
    var twitter_div = document.getElementById("twitter-embed")
    var iframe = document.getElementById("twitter-widget-0")
    var button = document.getElementById("twitter-button")
    var band = document.getElementById("twitter-band")
 
    if(button.style.left === "0%"){
        twitter_div.style.left = "0%";
        twitter_div.style.width = "75vw";
        twitter_div.style.height = "75vh";

        button.style.left = "75%"
        button.style.backgroundImage = "url('twitter_close.png')"
        button.style.transition = "all 0.5s ease-in-out"

        band.style.left = "0%";
        band.style.transition = "all 0.5s ease-in-out"

        iframe.style.width = "75vw";
        iframe.style.height = "75vh";
    }
    else{
        twitter_div.style.left = "-50%";
        twitter_div.style.width = "0vw";
        twitter_div.style.height = "0vh";

        button.style.backgroundImage = "url('twitter_open.png')"
        button.style.transition = "all 0.8s ease-in-out"

        band.style.left = "-75%";
        band.style.transition = "all 0.8s ease-in-out"

        button.style.left = "0%"
        iframe.style.width = "75vw";
        iframe.style.height = "75vh";
    }
    
}
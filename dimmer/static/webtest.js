

function slideChange(val, name) {
    var xhttp = new XMLHttpRequest();
    xhttp.open('POST', 'slide');
    xhttp.send('slider=' + name + '&value=' + val);
}

  function update_range( index, value ) {
    document.getElementById("myRange"+index).value = value;
    document.getElementById("demo"+index).value = value;
    slideChange( value, index)
  }



  function set_ref(event) {
    update_range(1,  document.getElementById("myRange1_ref").value );
    update_range(2,  document.getElementById("myRange2_ref").value );
    update_range(3,  document.getElementById("myRange3_ref").value );
    update_range(4,  document.getElementById("myRange4_ref").value );
    update_range(5,  document.getElementById("myRange5_ref").value );
    update_range(6,  document.getElementById("myRange6_ref").value );
    event.preventDefault(); // stop the form submit
  }


  function load_ref( value ) {

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            scene_data = JSON.parse(this.responseText);
            scene = scene_data.Scene;

            document.getElementById("scene_name").value = scene.name;
            document.getElementById("myRange1_ref").value = scene.levels[0];
            document.getElementById("myRange2_ref").value = scene.levels[1];
            document.getElementById("myRange3_ref").value = scene.levels[2];
            document.getElementById("myRange4_ref").value = scene.levels[3];
            document.getElementById("myRange5_ref").value = scene.levels[4];
            document.getElementById("myRange6_ref").value = scene.levels[5];

            if (scene.uuid == "00000000-0000-0000-0000-000000000000") {
                document.getElementById("update_disabler").classList.add("disabled");
                document.getElementById("scene_name").disabled = true;
                document.getElementById("updateButton").disabled = true;
                document.getElementById("deleteButton").disabled = true;
            } else {
                document.getElementById("update_disabler").classList.remove("disabled");
                document.getElementById("scene_name").disabled = false;
                document.getElementById("updateButton").disabled = false;
                document.getElementById("deleteButton").disabled = false;
            }
        }
    };

    xhttp.open('GET', '/json/scene?uuid='+value);
    xhttp.send();
  }


  function watchButtons2( ) {

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            levels = JSON.parse(this.responseText);

            document.getElementById("myRange1").value = levels[0];
            document.getElementById("myRange2").value = levels[1];
            document.getElementById("myRange3").value = levels[2];
            document.getElementById("myRange4").value = levels[3];
            document.getElementById("myRange5").value = levels[4];
            document.getElementById("myRange6").value = levels[5];

            watchButtons2()

        }
    };

    xhttp.open('GET', '/json/long_poll_buttons?rand='+Math.random());
      // random stops strange Chrome stalling with several outstanding requests
    xhttp.send();
  }


function sleep(ms) {
  return new Promise( resolve => setTimeout(resolve, ms) );
}


function get( path ) {
    return new Promise( function ( resolve, reject ) {
        var xhttp = new XMLHttpRequest();
        xhttp.open('GET', path);
        xhttp.onload = function() {
            if (this.status == 200) {
                resolve( this.responseText );
            } else {
                reject( Error(xhttp.statusText) );
            }
        };
        xhttp.onerror = function() {
              reject( Error("network error") );
          };
        xhttp.send();
    });
}


  async function watchButtons( ) {

      function update( levels) {
            //levels = JSON.parse(responseText);
            document.getElementById("myRange1").value = levels[0];
            document.getElementById("myRange2").value = levels[1];
            document.getElementById("myRange3").value = levels[2];
            document.getElementById("myRange4").value = levels[3];
            document.getElementById("myRange5").value = levels[4];
            document.getElementById("myRange6").value = levels[5];
        }
      async function fail( message ) {
          await sleep(1000); // rate limit
       }

    while (1) {
        await get('/json/long_poll_buttons?rand='+Math.random()).then(JSON.parse).then( update, fail );
          // random stops Chrome stalling with several outstanding requests for same URL
  }
}



function buttonPress(count) {
    var xhttp = new XMLHttpRequest();
    xhttp.open('POST', 'control');
    xhttp.send('button=' + count);
}




var slider1 = document.getElementById("myRange1");
var output1 = document.getElementById("demo1");
output1.value = slider1.value;
slider1.oninput = function() { output1.value = this.value; slideChange(this.value, "1") }
output1.oninput = function() { slider1.value = this.value || 0; slideChange(this.value, "1") }

var slider2 = document.getElementById("myRange2");
var output2 = document.getElementById("demo2");
output2.value = slider2.value;
slider2.oninput = function() { output2.value = this.value; slideChange(this.value, "2") }
output2.oninput = function() { slider2.value = this.value || 0; slideChange(this.value, "2") }

var slider3 = document.getElementById("myRange3");
var output3 = document.getElementById("demo3");
output3.value = slider3.value;
slider3.oninput = function() { output3.value = this.value; slideChange(this.value, "3") }
output3.oninput = function() { slider3.value = this.value || 0; slideChange(this.value, "3") }

var slider4 = document.getElementById("myRange4");
var output4 = document.getElementById("demo4");
output4.value = slider4.value;
slider4.oninput = function() { output4.value = this.value; slideChange(this.value, "4") }
output4.oninput = function() { slider4.value = this.value || 0; slideChange(this.value, "4") }

var slider5 = document.getElementById("myRange5");
var output5 = document.getElementById("demo5");
output5.value = slider5.value;
slider5.oninput = function() { output5.value = this.value; slideChange(this.value, "5") }
output5.oninput = function() { slider5.value = this.value || 0; slideChange(this.value, "5") }

var slider6 = document.getElementById("myRange6");
var output6 = document.getElementById("demo6");
output6.value = slider6.value;
slider6.oninput = function() { output6.value = this.value; slideChange(this.value, "6") }
output6.oninput = function() { slider6.value = this.value || 0; slideChange(this.value, "6") }



// function loadStatus1() {
//     var xhttp = new XMLHttpRequest();
//     xhttp.onreadystatechange = function() {
//         if (this.readyState == 4 && this.status == 200) {
//             document.getElementById('status1').innerHTML = this.responseText;
//             loadStatus1()
//         }
//     };
//     xhttp.open('GET', '/json/status1.txt', true);
//     xhttp.send();
// }
// function loadStatus2() {
//     var xhttp = new XMLHttpRequest();
//     xhttp.onreadystatechange = function() {
//         if (this.readyState == 4 && this.status == 200) {
//             document.getElementById('status2').innerHTML = this.responseText;
//             loadStatus2()
//         }
//     };
//     xhttp.open('GET', '/json/status2.txt', true);
//     xhttp.send();
// }




function loadStatus() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById('status').innerHTML = this.responseText;
        }
    };
    xhttp.open('GET', '/json/status.txt', true);
    xhttp.send();
//    setTimeout(loadStatus,1000);
}
window.addEventListener('load', function(){ loadStatus(); }, false);
window.addEventListener('load', function(){ watchButtons(); }, false);

document.getElementById("set_live").addEventListener("click", set_ref );


function buttonDown( code ) {

    fn = function() {
        console.log( this.code );
        buttonPress( this.code );
        this.repeat = setTimeout( fn, 200 );
    }

    console.log( code );
    buttonPress( code );
    this.code = code.toLowerCase();
    this.repeat = setTimeout( fn, 1000 );
}

function buttonUp( ) {
    clearTimeout( this.repeat );
}

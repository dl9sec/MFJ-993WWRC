//  Copyright (C) 2022 Thorsten Godau DL9SEC
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

setInterval(renewData, 800);
renewData();

function timeout(ms, promise) {
	return new Promise((resolve, reject) => {
		const timer = setTimeout(() => {
			reject(new Error('TIMEOUT'))
		}, ms)

		promise
			.then(value => {
				clearTimeout(timer)
				resolve(value)
			})
			.catch(reason => {
				clearTimeout(timer)
				reject(reason)
			})
		})
}


function renewData(){

	var imgLEDgn = document.getElementById("idLEDgn");
	var imgLEDrd = document.getElementById("idLEDrd");
	var imgWLAN  = document.getElementById("idWLAN");

	// Fetch JSON data with 3000ms timeout until .catch
	timeout(3000, fetch('/jsondata'))
	.then(
		function(response){
			return response.json();
		}
	)
	.then( 
		function(jsonArray){
			for ( var i in jsonArray){
				if (document.getElementById(i)) document.getElementById(i).innerHTML = jsonArray[i];
			} 

			//document.getElementById("varDebug").innerHTML = "|" + "Debug" + "|";

			var strIsTuning = document.getElementById("u8isTuning").innerHTML;
			var strSWRok    = document.getElementById("u8SWRok").innerHTML;
						
			if ( strIsTuning == "1") {
				// Red tuning LED on
				imgLEDrd.src="data:image/png;base64,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";
			}
			else {
				// Red tuning LED off
				imgLEDrd.src="data:image/png;base64,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";
			}

			if ( strSWRok == "1") {
				// Green SWR LED on
				imgLEDgn.src="data:image/png;base64,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";
			}
			else {
				// Green SWR LED off
				imgLEDgn.src="data:image/png;base64,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";
			}

			// If fetch() was succesfull, show WLAN symbol
			imgWLAN.src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAQCAMAAAAYoR5yAAAAB3RJTUUH5AoVBxAq+ECBOgAAAAlwSFlzAAAK8AAACvABQqw0mAAAAwBQTFRF/90A////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////BheKjQAAAAJ0Uk5T/wDltzBKAAAAM0lEQVR42mNghAMGEAYCMBNMgUgIA8rBrpaRgRFGM8CkGRA6kBShmYCsAG4YMhNJARQAACuzAH9Nr6EUAAAAAElFTkSuQmCC";
		})
		.catch(function(error) {
			// If fetch() failed, show NOWLAN symbol
			imgWLAN.src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAQCAMAAAAYoR5yAAAAB3RJTUUH5AoVBw4ptQjvXwAAAAlwSFlzAAAK8AAACvABQqw0mAAAAwBQTFRF/90A////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////BheKjQAAAAJ0Uk5T/wDltzBKAAAAMUlEQVR42mNghAMGIEQwGCAsMI2ggARMApmJqg5CM6CZCteGxEQygYGBAZOJbBsEAAAulACF2fQ6ugAAAABJRU5ErkJggg==";
			});
  
}

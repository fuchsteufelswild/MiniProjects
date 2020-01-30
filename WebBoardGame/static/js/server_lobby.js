var selectedGame = -1;
var in_game = 0


function getGameInfo(csrf_token, gameID, updateFunc) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&game_id=' + gameID + '&join=0')
  // req.responseType = 'text';
  if(req.status == 200) {
    //console.log(req.response)
    //console.log(String(req.responseText))
    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}


function updateGameList(csrf_token, updateFunc) {
  // console.log(selectedGame)
  var req = new XMLHttpRequest()
  req.open('GET', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('')
  if(req.status == 200) {
    var index1 = req.responseText.search('<body>')
    var index2 = req.responseText.search('</body>')
    document.body.innerHTML = req.responseText.slice(index1 + 6, index2)
    if(selectedGame != -1) {
      getGameInfo(csrf_token, selectedGame + 1, updateFunc)
    }
  }
}


function updateRoomInfo(rows, target) {
  if(!target){
    return;
  }

}

function updateGameInfo(resp) {
  if(typeof resp == 'string'){
    resp = JSON.parse(String(resp))
  }

  if(event && event.currentTarget) {
    selectedGame = parseInt(event.currentTarget.id[event.currentTarget.id.length - 1]) - 1
  }
  var rows = $("tr")

  for(var i = 0; i < rows.length; i++) {
    if(window.location.href.search("server_lobby") == -1){
      break;
    }
    if(i == selectedGame) {
      rows[i].style.backgroundcolor = 'rgba(255, 140, 130, 0.8)'
    }
    else {
      rows[i].style.backgroundColor = 'rgba(10, 140, 130, 0.1)'
    }
  }

  var target = $('#game_info')
  target.innerHTML = ""
  $('#dice').html("Dice Value: 0-" + String(resp.dice))
  $('#cycles').html("Cycles: " + String(resp.cycles))
  if(resp.credit > 0) {
    $('#credit').html("Starting Credit: " + String(resp.credit))
  }
  $('#credit').html("Starting Credit: 0")
  if(typeof resp.termination == 'string') {
    $('#termination').html("Termination: " + String(resp.termination))
  }
  else {
    for(var key in resp.termination) {
      $('#termination').html("Termination: " + key + "-" + resp.termination[key])
    }
  }

  $('div').remove(".square")
  var grid = $('#cell_grid')

  var total_len = resp.cells.length;
  var row_len = 5;
  var height = (total_len / 2) - 5;
  var alt = 0
  var pos = 0
  var offset;
  for(var i in resp.cells) {
    var node = document.createElement('div');
    if(alt > 0 && alt < height + 1) {
      var tNode = document.createElement('div');
      tNode.className = 'square cell'
      tNode.textContent = String(resp.cells[total_len - alt].description)
      tNode.style.background = getColorForCell(resp.cells[total_len - alt])
      document.getElementById('cell_grid').appendChild(tNode);
      for(var j = 1; j < row_len - 1; ++j){
        var tempNode = document.createElement('div')
        tempNode.className = 'square'
        document.getElementById('cell_grid').appendChild(tempNode);
      }
      node.className = 'square cell'
      node.textContent = String(resp.cells[i].description)
      node.style.background = getColorForCell(resp.cells[i])
      document.getElementById('cell_grid').appendChild(node);
      alt += 1
      if(alt == height + 1) {
        //console.log(total_len + parseInt(i))
        offset = total_len + parseInt(i) - height
      }
    }
    else if(alt == height + 1) {
      node.className = 'square cell'
      node.textContent = String(resp.cells[offset - parseInt(i)].description)
      node.style.background = getColorForCell(resp.cells[offset - parseInt(i)])
      document.getElementById('cell_grid').appendChild(node);

      if(parseInt(i) == total_len - height - 1) {
        break
      }
    }
    else {
      node.className = 'square cell'
      node.textContent = String(resp.cells[parseInt(i)].description)
      node.style.background = getColorForCell(resp.cells[parseInt(i)])
      pos += 1;
      if(pos == row_len){
        alt = 1;
      }
      document.getElementById('cell_grid').appendChild(node);
    }

  }

  $('div').remove(".drawcard")
  var deck = $('#card_list')

  for(var i in resp.cards) {
    var card = document.createElement('div')
    card.className = "drawcard"
    card.textContent = String(resp.cards[parseInt(i)].description)
    card.style.background = getColorForCell(resp.cards[i])
    document.getElementById('card_list').appendChild(card)
  }
}


function getColorForCell(incomingCell) {
  var colors = ["#0000ff", "#ff0000", "#008800", "#00ff00", "#00ffff", "#999900", "#550000", "#ff00ff"]

  if(typeof incomingCell.artifact != 'undefined') {
    return colors[0]
  }
  else if(typeof incomingCell.action != 'undefined') {
    if(typeof incomingCell.action.drop != 'undefined') {
      return colors[1]
    }
    else if(typeof incomingCell.action.jump != 'undefined') {
      if(String(incomingCell.action.jump).includes("=")) {
        return colors[3]
      }
      else {
        return colors[2]
      }
    }
    else if(typeof incomingCell.action.drawcard != 'undefined') {
      return colors[4]
    }
    else if(typeof incomingCell.action.add != 'undefined') {
      return colors[5]
    }
    else if(typeof incomingCell.action.skip != 'undefined') {
      return colors[6]
    }
    else if(typeof incomingCell.action.pay != 'undefined') {
      return colors[7]
    }
  }
}



function joinGame(csrf_token) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&game_id=' + selectedGame + '&join=1')
  // req.responseType = 'text';
  if(req.status == 200) {

    if(req.responseText != './'){
      //updater.socket.send("j")
      // close the socket

      window.location.href = "../game_lobby/"
    }
    else {
      window.location.href = String(req.responseText)
    }
    return;
    //console.log(req.response)

    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}


function updateInGame(csrf_token) {
  var req = new XMLHttpRequest()
  req.open('GET', '../game_state/', false)
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send(null)
  // console.log(req.responseText)
  if(req.status == 200) {
    var req1 = new XMLHttpRequest()
    req1.open('GET', './', false)
    req1.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
    req1.setRequestHeader('X-CSRFToken', csrf_token);
    req1.send(null)
    if(req1.status == 200){

      var index1 = req1.responseText.search('<body>')
      var index2 = req1.responseText.search('</body>')
      // console.log(document.body.innerHTML)

      if(req1.responseText.search("Game Room") == -1 && in_game == 0){
        window.location.href = "../game_scene/"
      }
      else if(req1.responseText.search("Game List") != -1){
        window.location.href = "../server_lobby/"
      }
      else {
        document.body.innerHTML = req1.responseText.slice(index1 + 6, index2)
      }
      updateGameInfo(JSON.parse(String(req.responseText)))
    }
  }
}


function sendRollAnswer(csrf_token, game_id) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&roll_val=1&pick_val=0')
  // req.responseType = 'text';
  if(req.status == 200) {
    //console.log("Roll JSON")
    //console.log(req.responseText)
    //updateGameInfo(JSON.parse(String(req.responseText)));
    //return;
    var req1 = new XMLHttpRequest()
    req1.open('GET', './', false)
    //req.setRequestHeader("Content-Type", "application/json")
    req1.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
    req1.setRequestHeader('X-CSRFToken', csrf_token);
    req1.send(null)

    if(req1.status == 200) {

      var index1 = req1.responseText.search('<body>')
      var index2 = req1.responseText.search('</body>')
      // console.log(document.body.innerHTML)
      //console.log(req1.responseText.slice(index1 + 6, index2))
      document.body.innerHTML = req1.responseText.slice(index1 + 6, index2)
      // $('body').html = "fdjslakfjdsa"
      updateGameInfo(JSON.parse(String(req.responseText)))
    }

    return
    //console.log(req.response)

    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}

function sendPickAnswer(csrf_token, game_id) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&pick_val=1&roll_val=0')
  // req.responseType = 'text';
  if(req.status == 200) {
    //updateGameInfo(JSON.parse(String(req.responseText)));

    var req1 = new XMLHttpRequest()
    req1.open('GET', './', false)
    //req.setRequestHeader("Content-Type", "application/json")
    req1.setRequestHeader("Content-Type", "application/xml; charset=UTF-8");
    req1.setRequestHeader('X-CSRFToken', csrf_token);
    req1.send('')

    if(req.status == 200) {
      var index1 = req1.responseText.search('<body>')
      var index2 = req1.responseText.search('</body>')
      //console.log(document.body.innerHTML)
      //console.log(req1.responseText.slice(index1 + 6, index2))
      document.body.innerHTML = req1.responseText.slice(index1 + 6, index2)
      // $('body').html = "fdjslakfjdsa"
      updateGameInfo(JSON.parse(String(req.responseText)))
    }

    return
    //console.log(req.response)

    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}

function sendDontPickAnswer(csrf_token, game_id) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&pick_val=0&roll_val=0')
  // req.responseType = 'text';
  if(req.status == 200) {
    //updateGameInfo(JSON.parse(String(req.responseText)));

    var req1 = new XMLHttpRequest()
    req1.open('GET', './', false)
    //req.setRequestHeader("Content-Type", "application/json")
    req1.setRequestHeader("Content-Type", "application/xml; charset=UTF-8");
    req1.setRequestHeader('X-CSRFToken', csrf_token);
    req1.send('')

    if(req.status == 200) {
      var index1 = req1.responseText.search('<body>')
      var index2 = req1.responseText.search('</body>')
      //console.log(document.body.innerHTML)
      //console.log(req1.responseText.slice(index1 + 6, index2))
      document.body.innerHTML = req1.responseText.slice(index1 + 6, index2)
      // $('body').html = "fdjslakfjdsa"
      updateGameInfo(JSON.parse(String(req.responseText)))
    }

    return
    //console.log(req.response)

    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}

function indicateReady(csrf_token) {
  var req = new XMLHttpRequest()
  req.open('POST', './', false)
  //req.setRequestHeader("Content-Type", "application/json")
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
  req.setRequestHeader('X-CSRFToken', csrf_token);
  req.send('id=all&nres=100&ready_val=1')
  // req.responseType = 'text';
  if(req.status == 200) {
    stringForm = req.responseText
    if(stringForm.search("Game Room") == -1){
      window.location.href = "../game_scene/"
    }
    else {
      var req1 = new XMLHttpRequest()
      req1.open('GET', '../game_state/', false)
      req1.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
      req1.setRequestHeader('X-CSRFToken', csrf_token);
      req1.send(null)
      if(req1.status == 200) {
        var index1 = req.responseText.search('<body>')
        var index2 = req.responseText.search('</body>')
        document.body.innerHTML = req.responseText.slice(index1 + 6, index2)
        updateGameInfo(JSON.parse(String(req1.responseText)))
      }
    }

    return
    //console.log(req.response)

    var obj = JSON.parse(String(req.responseText));
    updateFunc(obj)
  } else {
    return undefined
  }
}


function myScript() {
  alert("fdsaj")
}

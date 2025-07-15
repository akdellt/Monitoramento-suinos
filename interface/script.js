const client = mqtt.connect('ws://broker.hivemq.com:8000/mqtt');

client.on('connect', () => {
  client.subscribe('suinosmonit/ambiente/temperatura');
  client.subscribe('suinosmonit/porcos/peso');
});

let tempMax = 30;
let tempAtual = 0;

const ctxGauge = document.getElementById('gaugeTemp').getContext('2d');
const gaugeChart = new Chart(ctxGauge, {
  type: 'doughnut',
  data: {
    labels: ['Temperatura', ''],
    datasets: [{
      data: [tempAtual, tempMax - tempAtual],
      backgroundColor: ['green', '#e0e0e0'],
      borderWidth: 0,
      circumference: 180,
      rotation: 270,
      cutout: '60%'
    }]
  },
  options: {
    responsive: false,
    plugins: {
      legend: { display: false },
      tooltip: { enabled: false },
      title: {
        display: true,
        text: '',
        padding: { top: 10, bottom: 10 },
        font: { size: 16 }
      }
    }
  }
});




client.on('message', (topic, message) => {
  if (topic === 'suinosmonit/ambiente/temperatura') {
    const temp = parseFloat(message.toString());
    document.getElementById('tempAtual').textContent = temp.toFixed(1);
    tempMax = parseFloat(document.getElementById('tempMax').value) || 30;

    gaugeChart.data.datasets[0].data = [temp, Math.max(0, tempMax - temp)];

    let cor = 'green';
    if (temp >= tempMax) cor = 'red';
    else if (temp >= tempMax * 0.7) cor = 'orange';
    gaugeChart.data.datasets[0].backgroundColor = [cor, '#e0e0e0'];

    gaugeChart.options.plugins.title.text = `Temp: ${temp.toFixed(1)}°C`;
    gaugeChart.update();
  }

  if (topic === 'suinosmonit/porcos/peso') {
    const dados = JSON.parse(message.toString());
    adicionarLinhaTabela(dados);
  }
});

function publicarTemperaturaLimite() {
  const valor = document.getElementById('tempMax').value;
  client.publish('suinosmonit/controle/temperatura_max', valor);
}

function publicarHorario(tipo) {
  let valor = '';
  let topico = '';

  if (tipo === 'racao1') {
    valor = document.getElementById('horaRacao1').value;
    topico = 'suinosmonit/controle/racao/horario1';
  } else if (tipo === 'racao2') {
    valor = document.getElementById('horaRacao2').value;
    topico = 'suinosmonit/controle/racao/horario2';
  } else if (tipo === 'limpeza') {
    valor = document.getElementById('horaLimpeza').value;
    topico = 'suinosmonit/controle/limpeza/horarios';
  }

  if (valor) {
    client.publish(topico, valor);
  }
}

function acionarManual(tipo) {
  const topico = `suinosmonit/controle/${tipo}/status`;
  client.publish(topico, '1');
}

function adicionarLinhaTabela({ animal, peso, data }) {
  const tabela = document.getElementById('tabelaPorcos');
  const linha = document.createElement('tr');
  linha.innerHTML = `<td>${animal}</td><td>${peso}</td><td>${data}</td>`;
  tabela.appendChild(linha);
}

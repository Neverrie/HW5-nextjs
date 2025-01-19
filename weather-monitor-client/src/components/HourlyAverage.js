import React from 'react';

function HourlyAverage({ average }) {
  return (
    <div>
      <h2>Средняя температура за час</h2>
      <p>{average ? `${average} °C` : 'Загрузка...'}</p>
    </div>
  );
}

export default HourlyAverage;
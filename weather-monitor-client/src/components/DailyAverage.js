import React from 'react';

function DailyAverage({ average }) {
  return (
    <div>
      <h2>Средняя температура за день</h2>
      <p>{average ? `${average} °C` : 'Загрузка...'}</p>
    </div>
  );
}

export default DailyAverage;
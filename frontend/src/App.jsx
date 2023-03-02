import React, { useState } from 'react';
import { useForm } from "react-hook-form";
import useSWR from 'swr';

function fetcher(url, ...args) {
  return fetch(`${import.meta.env.VITE_SERVER}${url}`, ...args).then(res => {
    if(!res.ok) {
      return res.text().then(text => { throw new Error(text) })
     }
    else {
     return res.json();
   }
  });
}

async function postData(url = "", data = {}) {
  const response = await fetch(`${import.meta.env.VITE_SERVER}${url}`, {
    method: "POST",
    cache: "no-cache",
    headers: {
      "Content-Type": "application/json",
    },
    redirect: "follow",
    body: JSON.stringify(data),
  });
  return response.json();
}

function formatTime(time) {
  var minutes = Math.floor(time / 60);
  var seconds = Math.floor(time - minutes * 60);

  if (minutes < 10) {minutes = "0"+minutes;}
  if (seconds < 10) {seconds = "0"+seconds;}
  return minutes+':'+seconds;
}

export function AdminPage() {
  return <div className="container mx-auto p-5">
    <h1>Časoprostorová sešívačka</h1>
    <div className='w-full my-4'>
    <GameAgenda/>
    </div>
    <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
      <ButtonsAgenda/>
      <LanternAgenda/>
      <LaserAgenda/>
      <ChainAgenda/>
    </div>
  </div>;
}

function Spinner() {
  return <div role="status">
  <svg aria-hidden="true" className="w-8 h-8 mr-2 text-gray-200 animate-spin dark:text-gray-600 fill-blue-600" viewBox="0 0 100 101" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M100 50.5908C100 78.2051 77.6142 100.591 50 100.591C22.3858 100.591 0 78.2051 0 50.5908C0 22.9766 22.3858 0.59082 50 0.59082C77.6142 0.59082 100 22.9766 100 50.5908ZM9.08144 50.5908C9.08144 73.1895 27.4013 91.5094 50 91.5094C72.5987 91.5094 90.9186 73.1895 90.9186 50.5908C90.9186 27.9921 72.5987 9.67226 50 9.67226C27.4013 9.67226 9.08144 27.9921 9.08144 50.5908Z" fill="currentColor"/>
      <path d="M93.9676 39.0409C96.393 38.4038 97.8624 35.9116 97.0079 33.5539C95.2932 28.8227 92.871 24.3692 89.8167 20.348C85.8452 15.1192 80.8826 10.7238 75.2124 7.41289C69.5422 4.10194 63.2754 1.94025 56.7698 1.05124C51.7666 0.367541 46.6976 0.446843 41.7345 1.27873C39.2613 1.69328 37.813 4.19778 38.4501 6.62326C39.0873 9.04874 41.5694 10.4717 44.0505 10.1071C47.8511 9.54855 51.7191 9.52689 55.5402 10.0491C60.8642 10.7766 65.9928 12.5457 70.6331 15.2552C75.2735 17.9648 79.3347 21.5619 82.5849 25.841C84.9175 28.9121 86.7997 32.2913 88.1811 35.8758C89.083 38.2158 91.5421 39.6781 93.9676 39.0409Z" fill="currentFill"/>
  </svg>
  <span className="sr-only">Loading...</span>
</div>;
}

function Error({error}) {
  return <span className='text-red-500'>{error.toString()}</span>
}

function YesNo({value}) {
  return value ? <span className='bg-green-500 px-4 py-1 rounded-md'>Ano</span> : <span className='bg-red-500 px-4 py-1 rounded-md'>Ne</span>
}

function Card({className, children}) {
  return <div className={`w-full rounded-lg bg-white p-3 shadow-md ${className}`}>
    {children}
  </div>;
}

function GameAgenda() {
  const { data, error, isLoading, mutate } = useSWR(`/state`, fetcher, {
    refreshInterval: 1000,
    dedupingInterval: 1000
  });
  if (error)
    return <Error error={error}/>
  if (isLoading)
    return <Spinner/>
  return <Card className="w-full">
    <div className='w-full grid grid-cols-1 md:grid-cols-2 gap-4'>
      <div className='w-full'>
      <h2>Stav hry</h2>
        {
          data.running && <div className='inline-block w-full h-28 text-center rounded text-5xl p-8 align-middle m-auto bg-green-500'>Běží ({formatTime(data.time - data.start)})</div>
        }
        {
          !data.running && !data.restarting && <div className='inline-block w-full h-28 text-center rounded text-5xl p-8 align-middle m-auto bg-yellow-500'>Zastaveno</div>
        }
        {
          !data.running && data.restarting  && <div className='inline-block w-full h-28 text-center rounded text-5xl p-8 align-middle m-auto bg-red-500'>Restartuje se</div>
        }
      </div>
      <div className=''>
      <h2>Ovládání hry</h2>
      <button className='w-full py-1 my-1 bg-green-500' onClick={() => postData("/start").then(mutate)}>Spustit</button>
      <button className='w-full py-1 my-1 bg-yellow-500' onClick={() => postData("/stop").then(mutate)}>Zastavit</button>
      <button className='w-full py-1 my-1 bg-red-500' onClick={() => postData("/restart").then(mutate)}>Začít restartovat</button>
      </div>
    </div>

  </Card>
}

function ButtonsAgenda(){
  return <Card>
    <ButtonsSettings/>
    <hr className='my-3'/>
    <h2>Tlačítka</h2>
    <ButtonsOverview/>
  </Card>
}

const Input = React.forwardRef(({dirty, className, ...props}, ref) => {
  let cls = "bg-gray-50 border border-gray-300 text-gray-900 rounded-lg focus:ring-blue-500 focus:border-blue-500 block p-2.5 dark:bg-gray-700 dark:border-gray-600 dark:placeholder-gray-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500"
  if (dirty)
    cls += " border-orange-500"
  return <input ref={ref} className={`${cls} ${className}`} {...props}/>
})

function ButtonsSettings() {
  const [lastData, setLastData] = useState();
  const [disabled, setDisabled] = useState(false);
  const [stateMessage, setStateMessage] = useState();
  const { register, handleSubmit, reset, formState: { errors, isDirty, dirtyFields } } = useForm();

  const { data, error, isLoading, mutate } = useSWR(`/buttons/settings`, fetcher, {
    refreshInterval: 100,
    onSuccess: data => {
      // Meth, hacky
      if (JSON.stringify(data) == JSON.stringify(lastData))
        return
      setLastData(data);
      reset(data, {keepDefaultValues: true});
    }
  });


  const onSubmit = data => {
    setDisabled(true);
    setStateMessage("Odesílám");
    postData("/buttons/settings", data).finally( () => {
      setStateMessage("Uloženo!");
      setTimeout(() => setStateMessage(undefined), 2000);
      setDisabled(false);
      mutate();
    });
  }

  const fields = {
    pressTolerance: "Tolerance stisku",
    activityTolerance: "Timeout pro keep-alive",
    revealDuration: "Doba zobrazení",
    message: "Tajná zpráva"
  };

  return <div>
    <h2>Nastavení tlačítek</h2>
    <form onSubmit={handleSubmit(onSubmit)}>
      {
        Object.keys(fields).map(name => {
          return <div key={name} className='row flex items-center my-1'>
          <div className='w-1/3 text-right px-4'>
            <label>{fields[name]}</label>
          </div>
          <div className='w-2/3'>
          <Input className={"w-full"}  {...register(name)} dirty={dirtyFields[name] && false} type={name == "message" ? "text" : "number"}/>
          </div>
        </div>
        })
      }
      <input disabled={disabled} type={"submit"} className="w-full bg-purple-500 disabled:bg-gray-500 rounded-lg my-2 p-2 border-none shadow-sm" value={
        stateMessage ? stateMessage : "Uložit"
      }/>
    </form>
  </div>
}

function ButtonsOverview() {
  const { data, error, isLoading } = useSWR(`/buttons/state`, fetcher, {
    refreshInterval: 500,
    dedupingInterval: 500
  });

  if (isLoading)
    return <Spinner/>;
  if (error)
    return <Error error={error}/>
  return <div><table className='w-full'>
    <tbody>
    <tr>
      <td className='font-bold w-1/3 text-right px-4'>Odhaleno:</td>
      <td><YesNo value={data.revealed}/></td>
      <td/>
    </tr>
    <tr>
      <td className="font-bold text-right px-4">Aktivní tlačítka:</td>
      <td>{Object.keys(data.buttons).length}</td>
    </tr>
    <tr>
      <td></td>
      <td>Aktivní před</td>
      <td>Stisknuto před</td>
    </tr>
    {
      Object.keys(data.buttons).map(id => {
        let button = data.buttons[id];
        return <tr key={id}>
          <td className="font-bold w-1/3 text-right px-4">{id}:</td>
          <td>{(data.time - button.lastActive).toFixed(1)} s</td>
          <td>{ button.lastPress == 0 ? "Nebylo stisknuto" : `${(data.time - button.lastPress).toFixed(1)}s`}</td>
        </tr>
      })
    }
    </tbody>
  </table>
  </div>
}

function LaserAgenda(){
  const { data, error, isLoading } = useSWR(`/chain/state`, fetcher, {
    refreshInterval: 500,
    dedupingInterval: 500
  });

  if (isLoading)
    return <Spinner/>;
  if (error)
    return <Error error={error}/>

  let activeBefore = data.time - data.lastActive;
  let pinging = activeBefore < 5;
  return <Card>
    <h2>Laser</h2>
    <table className='w-full'>
    <tbody>
    <tr>
      <td>Zaregistrován</td>
      <td><YesNo value={pinging}/> { pinging && <>({formatTime(activeBefore)})</>}</td>
    </tr>
    <tr>
      <td>Aktivován</td>
      <td><YesNo value={data.enabled && pinging}/></td>
    </tr>
    </tbody>
    </table>
  </Card>
}

function LanternAgenda(){
  return <Card>
    <LanternSettings/>
    <hr className='my-2'/>
    <LanternOverview/>
  </Card>
}

function LanternSettings() {
  const [lastData, setLastData] = useState();
  const [disabled, setDisabled] = useState(false);
  const [stateMessage, setStateMessage] = useState();
  const { register, handleSubmit, reset, formState: { errors, isDirty, dirtyFields } } = useForm();

  const { data, error, isLoading, mutate } = useSWR(`/lanterns/settings`, fetcher, {
    refreshInterval: 1000,
    onSuccess: data => {
      // Meth, hacky
      if (JSON.stringify(data) == JSON.stringify(lastData))
        return
      setLastData(data);
      reset(data, {keepDefaultValues: true});
    }
  });

  if (error)
    return <><h2>Nastavení luceren</h2><Error error={error}/></>


  const onSubmit = data => {
    setDisabled(true);
    setStateMessage("Odesílám");
    postData("/lanterns/settings", data).finally( () => {
      setStateMessage("Uloženo!");
      setTimeout(() => setStateMessage(undefined), 2000);
      setDisabled(false);
      mutate();
    });
  }
  return <>
    <h2>Nastavení luceren</h2>
    <form onSubmit={handleSubmit(onSubmit)}>
    {
      [1, 2, 3].map(id =>
        <div key={id} className='row flex items-center my-1'>
              <div className='w-1/3 text-right px-4'>
                <label>Interval {id} (start, trvání)</label>
              </div>
              <div className='w-1/3'>
                <Input className={"w-full"}  {...register(`window${id}start`)} dirty={dirtyFields[`window${id}start`] && false} type="number"/>
              </div>
              <div className='w-1/3'>
                <Input className={"w-full"}  {...register(`window${id}duration`)} dirty={dirtyFields[`window${id}duration`] && false} type="number"/>
              </div>
            </div>
      )
    }
    <input disabled={disabled} type={"submit"} className="w-full bg-purple-500 disabled:bg-gray-500 rounded-lg my-2 p-2 border-none shadow-sm" value={
        stateMessage ? stateMessage : "Uložit"
      }/>
    </form>
  </>;
}

function LanternOverview() {
  const { data, error, isLoading } = useSWR(`/lanterns/state`, fetcher, {
    refreshInterval: 500,
    dedupingInterval: 500
  });

  if (isLoading)
    return <Spinner/>;
  if (error)
    return <Error error={error}/>

  return <>
    <h2>Přehled luceren</h2>
    <table className='w-full'>
    <tbody>
    <tr>
      <td>Lucerna</td>
      <td>Aktivní před</td>
      <td>Baterie</td>
    </tr>
    {
      Object.keys(data.lanterns).map(id => {
        let lantern = data.lanterns[id];
        return <tr key={id}>
          <td className="font-bold w-1/3 text-right px-4">{id}:</td>
          <td>{(data.time - lantern.lastActive).toFixed(1)} s</td>
          <td>{lantern.battery} %</td>
        </tr>
      })
    }
    </tbody>
  </table>
  </>;
}

function ChainAgenda(){
  const { data, error, isLoading } = useSWR(`/chain/state`, fetcher, {
    refreshInterval: 500,
    dedupingInterval: 500
  });

  if (isLoading)
    return <Spinner/>;
  if (error)
    return <Error error={error}/>

  let activeBefore = data.time - data.lastActive;
  let pinging = activeBefore < 5;

  return <Card>
    <h2>Lidský řetěz</h2>
    <table className='w-full'>
    <tbody>
    <tr>
      <td>Zaregistrován</td>
      <td><YesNo value={pinging}/> { pinging && <>({formatTime(activeBefore)})</>}</td>
    </tr>
    <tr>
      <td>Aktivován</td>
      <td><YesNo value={data.active && pinging}/></td>
    </tr>
    </tbody>
    </table>
  </Card>
}

export function ButtonsPage() {
  const { data, error, isLoading } = useSWR(`/buttons/state`, fetcher, {
    refreshInterval: 500,
    dedupingInterval: 500
  });

  if (isLoading)
    return <Spinner/>;
  if (error)
    return <Error error={error}/>
  return <div className='container mx-auto pt-28'>
    <Card className="text-6xl font-bold p-28 text-center
    ">{data.message ? data.message : "Zde není nic k vidění."}
    </Card></div>
}


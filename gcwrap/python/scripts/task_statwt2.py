from taskinit import mstool, tbtool, casalog, write_history
import flaghelper

def statwt2(
    vis, selectdata, field, spw, intent, array, observation, combine,
    timebin, slidetimebin, chanbin, minsamp, statalg, fence, center,
    lside, zscore, maxiter, excludechans, wtrange,
    flagbackup, preview, datacolumn
):
    casalog.origin('statwt2')
    casalog.post("Warning: the task statwt2 has undergone significant development and will be called statwt starting in CASA 5.4.", "WARN")
    if not selectdata:
        # CAS-10761, requirement provided by Urvashi
        if field or spw or intent or array or observation:
            casalog.post(
                "selectdata=False, any explicitly set data "
                + "selection parameters will be ignored",
                "WARN"
            )
        field = ""
        spw = ""
        intent = ""
        array = ""
        observation = ""
    try:
        if (flagbackup):
            if (preview):
                casalog.post(
                    "Running in preview mode. No flags will be "
                    + "modified, so existing flags will not be backed up."
                )
            else:
                casalog.post('Backup original flags before applying new flags')
                flaghelper.backupFlags(aflocal=None, msfile=vis, prename='statwt2')
        myms = mstool()
        myms.open(vis, nomodify=preview)
        sel = {}
        sel['spw'] = spw
        #sel['time'] = timerange
        sel['field'] = field
        #sel['baseline'] = antenna
        #sel['scan'] = scan
        sel['scanintent'] = intent
        #sel['polarization'] = correlation
        #sel['uvdist'] = uvrange
        sel['observation'] = str(observation)
        sel['array'] = array
        #sel['feed'] = feed
        # Select the data. Only-parse is set to false.
        myms.msselect(sel, False)
        return myms.statwt2(
            combine=combine, timebin=timebin,
            slidetimebin=slidetimebin, chanbin=chanbin,
            minsamp=minsamp, statalg=statalg, fence=fence,
            center=center, lside=lside, zscore=zscore,
            maxiter=maxiter, excludechans=excludechans,
            wtrange=wtrange, preview=preview, datacolumn=datacolumn
        )
    except Exception, instance:
        casalog.post( '*** Error ***'+str(instance), 'SEVERE' )
        raise
    finally:
        myms.done()
